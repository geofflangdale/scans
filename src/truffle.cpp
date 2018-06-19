#include <memory>
#include <set>

#include "scans.h"
#include "truffle.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_truffle(const std::set<u8> & in) {
    return make_unique<Wrapper<Truffle>>(Truffle(in));
}
