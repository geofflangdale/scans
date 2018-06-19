#pragma once

#include <vector>
#include <set>
#include <x86intrin.h>
#include "common_defs.h"
#include "util.h"
#include "scans.h"

#include <iostream>

class Truffle {
    m256 lo;
    m256 hi;
    
public:
    Truffle(const std::set<u8> & in) {
        m128 lo_tmp = _mm_set1_epi8(0);
        m128 hi_tmp = _mm_set1_epi8(0);
        for (auto c : in) {
            u32 byte = c & 0x0f;
            u32 bit = (c & 0x70) >> 4;
            if (c & 0x80) {
                set_bit(hi_tmp, byte*8 + bit);
            } else {
                set_bit(lo_tmp, byte*8 + bit);
            }
        }
        lo = _mm256_broadcastsi128_si256(lo_tmp);
        hi = _mm256_broadcastsi128_si256(hi_tmp);
    }

    u32 truffle_op(m256 input) {
        const m256 log2_mask = _mm256_setr_epi8(
            1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128,
            1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128
        );
        m256 t1 = _mm256_shuffle_epi8(lo, input);
        m256 t2 = _mm256_shuffle_epi8(hi, _mm256_xor_si256(input, _mm256_set1_epi8(0x80)));
        m256 t3 = _mm256_or_si256(t1, t2);
        m256 t4 = _mm256_and_si256(t3, _mm256_shuffle_epi8( log2_mask, 
                                                _mm256_and_si256(_mm256_srli_epi32(input, 4),
                                                                 _mm256_set1_epi8(0x07))));
        m256 t5 = _mm256_cmpeq_epi8(t4, _mm256_set1_epi8(0));
        return ~(u32)_mm256_movemask_epi8(t5);
    }

    void scan(InputBlock input, std::vector<u32> & out, UNUSED std::vector<u8> & tmp) {
        apply_scanner_op<Truffle, &Truffle::truffle_op>(*this, input, out);
    }
};

