#include <memory>
#include <set>

#include "scans.h"
#include "literal_gold.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_literalgold(const std::vector<Literal> & in) {
    return make_unique<Wrapper<LiteralGold>>(LiteralGold(in));
}
