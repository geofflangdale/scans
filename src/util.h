#pragma once
#include <x86intrin.h>
#include "common_defs.h"

// non-performant set bit for compiler-code only
// a fast version would OR from a table
// no safety checking
template <typename T>
inline void set_bit(T & m, u32 bit) {
    union {
        T avx;
        u8 bytes[sizeof(T)];
    } u;
    u.avx = m;
    u.bytes[bit/8] |= (1 << bit%8);
    m = u.avx;
}
