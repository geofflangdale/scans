#pragma once
#include <utility>
#include <vector>
#include <chrono>
#include <tuple>
#include <set>
#include <ostream>
#include <fstream>
#include <iterator>
#include <cstring>
#include "common_defs.h"

#include <iostream>
typedef u32 OffsetResult;
typedef std::pair<u32, u32> OffsetIDResult;

inline std::ostream & operator<<(std::ostream & stream, const OffsetIDResult & r) {
    stream << r.first << "/" << r.second;
    return stream;
}

// result handling
class ResultBase {
protected:
    size_t size;
public:
    ResultBase(size_t size_in) : size(size_in) {}
    virtual ~ResultBase() {}
    virtual void dump_results(std::ostream &os) = 0;
    virtual bool operator!=(const ResultBase & rb) const = 0;
    
    virtual size_t get_size() = 0;
};

template<typename T> class Result : public ResultBase {
public:
    std::vector<T> results;

    Result(size_t size) : ResultBase(size) {
        results.resize(size); // T must have a default constructor
    }

    // trim result vector to fit
    void trim(size_t size) { 
        results.resize(size);
    }

    virtual void dump_results(std::ostream & os) {
        for (auto r : results) {
            os << r << "\n";
        }
    }

    virtual bool operator!=(const ResultBase & rb) const {
        const Result & r_other = dynamic_cast<const Result &>(rb); // throws bad_cast if this isn't allowed, which is what we want
        std::cout <<"here!\n";
        return results != r_other.results;
    }

    virtual size_t get_size() {
        return results.size();
    }
};


class InputBlock {
public:
    const u8 * buf;
    size_t len;
    size_t start; // where to start scanning in the block
    size_t end;   // where to finish scanning

    // These are a bit future-proofish. I think having a soft_start/soft_end make a good
    // deal of sense for using this BLSE formulation to (say) split a single scan up into
    // more tractable worst-case size scans (i.e. staying <4GB or <64KB

    bool hard_start = false; // hard_start means we can't look at 0..start-1 for context
    bool hard_end = false;   // hard_end means we can't look at end..len-1 for context

    InputBlock(const u8 * buf_in, size_t len_in, size_t start_in, size_t end_in) :
        buf(buf_in), len(len_in), start(start_in), end(end_in) {
        
        assert(start <= end);
        assert(end <= len);
    }


    size_t bytes_to_scan() {
        return end-start;
    }
};

template<typename R> 
inline void flatten_results(u64 res, u32 & result_idx, R & results, size_t idx, u32 adjust_downward = 0) {
    while (res) {
        results[result_idx++] = (u32)idx + __builtin_ctzll(res) - adjust_downward;
        res &= res - 1ULL;
    }
}

template<typename T, u32 (T::*op)(m256 input)>
inline u64 op64(T & scanner, const u8 * location) {
    m256 input_0 = _mm256_load_si256((const m256 *)(location));
    m256 input_1 = _mm256_load_si256((const m256 *)(location+32));
    return (scanner.*op)(input_0) | ((u64)(scanner.*op)(input_1) << 32);
}

template<typename T, u32 (T::*op)(m256 input)>
inline u64 op64u(T & scanner, const u8 * location) {
    m256 input_0 = _mm256_loadu_si256((const m256 *)(location));
    m256 input_1 = _mm256_loadu_si256((const m256 *)(location+32));
    return (scanner.*op)(input_0) | ((u64)(scanner.*op)(input_1) << 32);
}

template<typename T, u32 (T::*op)(m256 input)>
inline void apply_scanner_op(T & scanner, InputBlock input, Result<typename T::ResultType> & out) {
        const u8 * buf = input.buf;
        u32 result_idx = 0;
        
        size_t effective_length = input.end - input.start;
        if (effective_length <= 64) {
            u8 tmp[64] __attribute__((aligned(64)));
            memcpy(tmp + (64-effective_length), buf + input.start, effective_length);
            u64 res = op64<T, op>(scanner, &tmp[0]);
            res <<= (64-effective_length);
            flatten_results(res, result_idx, out.results, input.start, 0);
        } else {
            size_t idx = input.start;
            size_t effective_start_alignment = ((u64)(buf + idx)) & 0x3f;

            // warm up by processing to a cache line shifting our results to eliminate bogus results
            // if we aren't 64-aligned to start with, we need to process (64-effective_start_alignment)
            // bytes. We will process them from buf+start as we know we can process 64 bytes safely
            if (effective_start_alignment) {
                u64 res = op64u<T, op>(scanner, buf + idx);
                res <<= effective_start_alignment;
                flatten_results(res, result_idx, out.results, idx, effective_start_alignment);
                idx += 64-effective_start_alignment;
            }
            
            // main loop: process whole cache lines. There's about 10% (55->60) from a aggressive unroll
            // but it's pretty ridiculous past a point
            for (; idx + 63 < input.end; idx+=64) {
                __builtin_prefetch(buf + idx + 64*64);
                u64 res = op64<T, op>(scanner, buf + idx);
                flatten_results(res, result_idx, out.results, idx, 0);
            }

            if (idx < input.end) {
                // cool down. Read last 64 bytes of the buffer, scan them, and trim
                // off any spurious bits we've already seen
                u64 res = op64u<T, op>(scanner, buf + input.end - 64); 
                // trim off overlap
                res >>= (64 - (input.end - idx));
                flatten_results(res, result_idx, out.results, idx, 0);
            }

        }
        out.trim(result_idx);
}

// scanner1 is earlier, scanner2 is later, they are separated by distance (!=0)
template<typename T1, u32 (T1::*op1)(m256 input),
         typename T2, u32 (T2::*op2)(m256 input)>
inline void apply_double_scanner_op(T1 & scanner1, T2 & scanner2, u32 distance,
                                    InputBlock input, Result<typename T1::ResultType> & out) {
        const u8 * buf = input.buf;
        u32 result_idx = 0;

        u64 old_res_0 = 0;
        
        size_t effective_length = input.end - input.start;
        if (effective_length <= 64) {
            u8 tmp[64] __attribute__((aligned(64)));
            memcpy(tmp + (64-effective_length), buf + input.start, effective_length);
            u64 res_0 = op64<T1, op1>(scanner1, &tmp[0]);
            u64 res_1 = op64<T2, op2>(scanner2, &tmp[0]);
            u64 res = res_1 & ((res_0 << distance) | (old_res_0 >> (64-distance)));
            res <<= (64-effective_length);
            flatten_results(res, result_idx, out.results, input.start, 0);
        } else {
            size_t idx = input.start;
            size_t effective_start_alignment = ((u64)(buf + idx)) & 0x3f;

            // warm up by processing to a cache line shifting our results to eliminate bogus results
            // if we aren't 64-aligned to start with, we need to process (64-effective_start_alignment)
            // bytes. We will process them from buf+start as we know we can process 64 bytes safely
            if (effective_start_alignment) {
                u64 res_0 = op64u<T1, op1>(scanner1, buf + idx);
                u64 res_1 = op64u<T2, op2>(scanner2, buf + idx);
                u64 res = res_1 & ((res_0 << distance) | (old_res_0 >> (64-distance)));
                old_res_0 = res_0;
                res <<= effective_start_alignment;
                old_res_0 <<= effective_start_alignment;
                flatten_results(res, result_idx, out.results, idx, effective_start_alignment);
                idx += 64-effective_start_alignment;
            }
            
            // main loop: process whole cache lines. 
            for (; idx + 63 < input.end; idx+=64) {
                __builtin_prefetch(buf + idx + 64*64);
                u64 res_0 = op64u<T1, op1>(scanner1, buf + idx);
                u64 res_1 = op64u<T2, op2>(scanner2, buf + idx);
                u64 res = res_1 & ((res_0 << distance) | (old_res_0 >> (64-distance)));
                old_res_0 = res_0;
                flatten_results(res, result_idx, out.results, idx, 0);
            }

            if (idx < input.end) {
                // cool down. Read last 64 bytes of the buffer, scan them, and trim
                // off any spurious bits we've already seen
                u64 res_0 = op64u<T1, op1>(scanner1, buf + input.end - 64);
                u64 res_1 = op64u<T2, op2>(scanner2, buf + input.end - 64);
                u64 res = res_1 & ((res_0 << distance) | (old_res_0 >> (64-distance)));
                // trim off overlap
                res >>= (64 - (input.end - idx));
                flatten_results(res, result_idx, out.results, idx, 0);
            }

        }
        out.trim(result_idx);
}

class WrapperBase {
public:
    WrapperBase() {}
    virtual std::vector<double> benchmark(InputBlock input, u32 repeats) = 0;
    virtual ResultBase * log(InputBlock input) = 0;
};

template <typename T> class Wrapper : public WrapperBase {
    T scanner;
public:
    Wrapper(T scanner_in) : scanner(scanner_in) {}

    virtual std::vector<double> benchmark(InputBlock input, u32 repeats) {
        Result<typename T::ResultType> out(input.bytes_to_scan());
        std::vector<double> times;
        times.reserve(repeats);
        for (u32 i = 0; i < repeats; ++i) {
            auto start = std::chrono::steady_clock::now();
            scanner.scan(input, out);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> secs = end - start;
            times.push_back(secs.count());
        }
        return times;
    }

    virtual ResultBase * log(InputBlock input) {
        Result<typename T::ResultType> * out = new Result<typename T::ResultType>(input.bytes_to_scan());
        scanner.scan(input, *out);
        return out;
    }
};


typedef std::tuple<std::set<u8>, std::set<u8>, u32> DoubleCharsetWorkload;
