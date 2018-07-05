#include <memory>
#include <set>

#include "scans.h"
#include "nvs.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_nvs(const std::vector<Literal> & in) {
    return make_unique<Wrapper<NVS>>(NVS(in));
}
