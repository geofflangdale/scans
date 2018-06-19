#include "public.h"

#include <memory>
#include <set>

using namespace std;

unique_ptr<WrapperBase> get_wrapper(string name, const set<u8> & in) {
    if (name == "truffle") {
        return get_wrapper_truffle(in);
    } else if (name == "charsetgold") {
        return get_wrapper_charsetgold(in);
    } else if (name == "shufti") {
        return get_wrapper_shufti(in);
    } else if (name == "vermicelli") {
        return get_wrapper_vermicelli(in);
    }
    return 0;
}
