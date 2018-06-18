#include <memory>
#include <set>

#include "scans.h"
#include "truffle.h"

using namespace std;

std::unique_ptr<LoggerBase> get_logger_truffle(const std::set<u8> & in) {
    return make_unique<Logger<Truffle>>(Truffle(in));
}

std::unique_ptr<BenchmarkerBase> get_benchmarker_truffle(const std::set<u8> & in) {
    return make_unique<Benchmarker<Truffle>>(Truffle(in));
}

