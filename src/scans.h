#pragma once
#include <utility>
#include <vector>
#include <chrono>
#include "common_defs.h"

typedef std::pair<u8 *, size_t> InputBlock;

// any bulk bytes->bits->indexes operation scan can be phrased this way
// TODO: write the boring code that handles small inputs, proper warm-up and cooldown
template<typename T, u32 (T::*op)(m256 input)>
inline void apply_scanner_op(T & scanner, InputBlock input, std::vector<u32> & out) {
        u8 * buf = input.first;
        size_t len = input.second;
        u32 result_idx = 0;
        for (size_t idx = 0; idx < len; idx+=64) {
            __builtin_prefetch(buf + idx + 64*64);
            m256 input_0 = _mm256_load_si256((const m256 *)(buf + idx));
            m256 input_1 = _mm256_load_si256((const m256 *)(buf + idx + 32));
            u64 res_0 = (scanner.*op)(input_0);
            u64 res_1 = (u64)(scanner.*op)(input_1) << 32;
            u64 res = res_0 | res_1;
            while (res) {
                out[result_idx++] = (u32)idx + __builtin_ctzll(res);
                res &= res - 1ULL;
            }
        }
        out.resize(result_idx);
}

class WrapperBase {
public:
    WrapperBase() {}
    virtual std::vector<double> benchmark(InputBlock input, u32 repeats) = 0;
    virtual std::vector<u32> log(InputBlock input) = 0;
};

template <typename T> class Wrapper : public WrapperBase {
    T scanner;
public:
    Wrapper(T scanner_in) : scanner(scanner_in) {}

    virtual std::vector<double> benchmark(InputBlock input, u32 repeats) {
        std::vector<u32> out;
        std::vector<double> results;
        std::vector<u8> tmp;
        out.resize(input.second);
        tmp.resize(input.second);
        results.reserve(repeats);
        for (u32 i = 0; i < repeats; ++i) {
            auto start = std::chrono::steady_clock::now();
            scanner.scan(input, out, tmp);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> secs = end - start;
            results.push_back(secs.count());
        }
        return results;
    }

    virtual std::vector<u32> log(InputBlock input) {
        std::vector<u32> out;
        out.resize(input.second);
        std::vector<u8> tmp;
        out.resize(input.second);
        scanner.scan(input, out, tmp);
        return out;
    }
};

