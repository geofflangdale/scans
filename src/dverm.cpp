#include <memory>
#include <set>

#include "scans.h"
#include "dverm.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_dverm(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<DVerm>>(DVerm(in));
}
