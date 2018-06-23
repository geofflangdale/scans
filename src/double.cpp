#include <memory>
#include <set>

#include "scans.h"
#include "double.h"
#include "vermicelli.h"
#include "shufti.h"
#include "truffle.h"

using namespace std;

// note: we could make all 9 variations here, but the other 4 aren't that interesting

typedef DoubleMatcher<Vermicelli, &Vermicelli::vermicelli_op,
                      Vermicelli, &Vermicelli::vermicelli_op> DVerm;

typedef DoubleMatcher<Shufti, &Shufti::shufti_op,
                      Shufti, &Shufti::shufti_op> DShufti;

typedef DoubleMatcher<Truffle, &Truffle::truffle_op,
                      Truffle, &Truffle::truffle_op> DTruffle;

typedef DoubleMatcher<Shufti, &Shufti::shufti_op,
                      Vermicelli, &Vermicelli::vermicelli_op> ShufVerm;

typedef DoubleMatcher<Vermicelli, &Vermicelli::vermicelli_op,
                      Shufti, &Shufti::shufti_op> VermShuf;

std::unique_ptr<WrapperBase> get_wrapper_dverm(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<DVerm>>(DVerm(in));
}

std::unique_ptr<WrapperBase> get_wrapper_dshufti(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<DShufti>>(DShufti(in));
}

std::unique_ptr<WrapperBase> get_wrapper_dtruffle(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<DTruffle>>(DTruffle(in));
}

std::unique_ptr<WrapperBase> get_wrapper_shufverm(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<ShufVerm>>(ShufVerm(in));
}

std::unique_ptr<WrapperBase> get_wrapper_vermshuf(const DoubleCharsetWorkload & in) {
    return make_unique<Wrapper<VermShuf>>(VermShuf(in));
}
