#include <memory>
#include <set>

#include "scans.h"
#include "doubleset_gold.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_doublesetgold(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<DoublesetGold>>(DoublesetGold(in));
}
