#include <memory>
#include <set>

#include "scans.h"
#include "charset_gold.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_charsetgold(const std::set<u8> & in) {
    return make_unique<Wrapper<CharsetGold>>(CharsetGold(in));
}
