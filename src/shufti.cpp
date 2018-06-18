#include <memory>
#include <set>

#include "scans.h"
#include "shufti.h"

using namespace std;

std::unique_ptr<LoggerBase> get_logger_shufti(const std::set<u8> & in) {
    return make_unique<Logger<Shufti>>(Shufti(in));
}

std::unique_ptr<BenchmarkerBase> get_benchmarker_shufti(const std::set<u8> & in) {
    return make_unique<Benchmarker<Shufti>>(Shufti(in));
}

