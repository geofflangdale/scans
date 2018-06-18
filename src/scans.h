#pragma once

#include <utility>
#include <vector>
#include <chrono>
#include "common_defs.h"

typedef std::pair<u8 *, size_t> InputBlock;

class BenchmarkerBase {
public:
    BenchmarkerBase() {}
    virtual std::vector<double> benchmark(InputBlock input, u32 repeats) = 0;
};

class LoggerBase {
public:
    LoggerBase() {}
    virtual std::vector<u32> log(InputBlock input) = 0;
};

template <typename T> class Benchmarker : public BenchmarkerBase {
    T scanner;
public:
    Benchmarker(T scanner_in) : scanner(scanner_in) {}
    virtual std::vector<double> benchmark(InputBlock input, u32 repeats) {
        std::vector<u32> out;
        std::vector<double> results;
        out.resize(input.second);
        for (u32 i = 0; i < repeats; ++i) {
            auto start = std::chrono::steady_clock::now();
            scanner.scan(input, out);
            auto end = std::chrono::steady_clock::now();
            std::chrono::duration<double> secs = end - start;
            results.push_back(secs.count());
        }
        return results;
    }
};

template <typename T> class Logger : public LoggerBase {
public:
    T scanner;
public:
    Logger(T scanner_in) : scanner(scanner_in) {}
    virtual std::vector<u32> log(InputBlock input) {
        std::vector<u32> out;
        out.resize(input.second);
        scanner.scan(input, out);
        return out;
    }
};
