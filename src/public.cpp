#include "public.h"

#include <memory>
#include <set>

using namespace std;

std::unique_ptr<WrapperBase> get_wrapper_truffle(const std::set<u8> & in);
std::unique_ptr<WrapperBase> get_wrapper_shufti(const std::set<u8> & in);
std::unique_ptr<WrapperBase> get_wrapper_vermicelli(const std::set<u8> & in);
std::unique_ptr<WrapperBase> get_wrapper_vermlite(const std::set<u8> & in);

unique_ptr<WrapperBase> get_wrapper(string name, const set<u8> & in) {
    if (name == "truffle") {
        return get_wrapper_truffle(in);
    } else if (name == "charsetgold") {
        return get_wrapper_charsetgold(in);
    } else if (name == "shufti") {
        return get_wrapper_shufti(in);
    } else if (name == "vermicelli") {
        return get_wrapper_vermicelli(in);
    } else if (name == "vermlite") {
        return get_wrapper_vermlite(in);
    }
    return 0;
}
