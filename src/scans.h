#pragma once

#include <utility>
#include <vector>
#include <chrono>
#include "common_defs.h"

typedef std::pair<u8 *, size_t> InputBlock;

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

