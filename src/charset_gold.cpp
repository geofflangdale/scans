#include <memory>
#include <set>

#include "scans.h"
#include "charset_gold.h"

using namespace std;

std::unique_ptr<LoggerBase> get_logger_charsetgold(const std::set<u8> & in) {
    return make_unique<Logger<CharsetGold>>(CharsetGold(in));
}

std::unique_ptr<BenchmarkerBase> get_benchmarker_charsetgold(const std::set<u8> & in) {
    return make_unique<Benchmarker<CharsetGold>>(CharsetGold(in));
}

