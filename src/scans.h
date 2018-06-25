#pragma once
#include <utility>
#include <vector>
#include <chrono>
#include <tuple>
#include <set>
#include <ostream>
#include <fstream>
#include <iterator>
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


typedef std::pair<u8 *, size_t> InputBlock;

// any bulk bytes->bits->indexes operation scan can be phrased this way
// TODO: write the boring code that handles small inputs, proper warm-up and cooldown
template<typename T, u32 (T::*op)(m256 input)>
inline void apply_scanner_op(T & scanner, InputBlock input, Result<typename T::ResultType> & out) {
        u8 * buf = input.first;
        size_t len = input.second;
        u32 result_idx = 0;
        for (size_t idx = 0; idx < len; idx+=64) {
            __builtin_prefetch(buf + idx + 64*64);
            m256 input_0 = _mm256_load_si256((const m256 *)(buf + idx));
            m256 input_1 = _mm256_load_si256((const m256 *)(buf + idx + 32));
            u64 res = (scanner.*op)(input_0) | ((u64)(scanner.*op)(input_1) << 32);
            while (res) {
                out.results[result_idx++] = (u32)idx + __builtin_ctzll(res);
                res &= res - 1ULL;
            }
        }
        out.trim(result_idx);
}

// scanner1 is earlier, scanner2 is later, they are separated by distance (!=0)

template<typename T1, u32 (T1::*op1)(m256 input),
         typename T2, u32 (T2::*op2)(m256 input)>
inline void apply_double_scanner_op(T1 & scanner1, T2 & scanner2, u32 distance,
                                    InputBlock input, Result<typename T1::ResultType> & out) {
        u8 * buf = input.first;
        size_t len = input.second;
        u32 result_idx = 0;

        u64 old_res_scan_0 = 0;
        for (size_t idx = 0; idx < len; idx+=64) {
            __builtin_prefetch(buf + idx + 64*64);
            m256 input_0 = _mm256_load_si256((const m256 *)(buf + idx));
            m256 input_1 = _mm256_load_si256((const m256 *)(buf + idx + 32));
            u64 res_scan_0 = (scanner1.*op1)(input_0) | ((u64)(scanner1.*op1)(input_1) << 32);
            u64 res_scan_1 = (scanner2.*op2)(input_0) | ((u64)(scanner2.*op2)(input_1) << 32);

            u64 res = res_scan_1 & ((res_scan_0 << distance) | (old_res_scan_0 >> (64-distance)));
            old_res_scan_0 = res_scan_0;

            while (res) {
                out.results[result_idx++] = (u32)idx + __builtin_ctzll(res);
                res &= res - 1ULL;
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
        Result<typename T::ResultType> out(input.second);
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
        Result<typename T::ResultType> * out = new Result<typename T::ResultType>(input.second);
        scanner.scan(input, *out);
        return out;
    }
};


typedef std::tuple<std::set<u8>, std::set<u8>, u32> DoubleCharsetWorkload;
