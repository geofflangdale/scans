#include <memory>
#include <set>

#include "scans.h"
#include "shufti.h"

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_shufti(const std::set<u8> & in) {
    return make_unique<Wrapper<Shufti>>(Shufti(in));
}
