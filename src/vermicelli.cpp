#include <memory>
#include <set>

#include "scans.h"
#include "vermicelli.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_vermicelli(const std::set<u8> & in) {
    return make_unique<Wrapper<Vermicelli>>(Vermicelli(in));
}

std::unique_ptr<WrapperBase> get_wrapper_vermlite(const std::set<u8> & in) {
    return make_unique<Wrapper<VermLite>>(VermLite(in));
}
