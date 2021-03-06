#pragma once
#include <vector>
#include <set>
#include <iostream>
#include <stdexcept>
#include <x86intrin.h>
#include "common_defs.h"
#include "util.h"
#include "scans.h"

class Shufti {
    m256 lo;
    m256 hi;
public:
    typedef OffsetResult ResultType;

    Shufti(const std::set<u8> & in) {
        m128 lo_nibble_mask = _mm_set1_epi8(0);
        m128 hi_nibble_mask = _mm_set1_epi8(0);
        // better compiler would check for cases where we have <8 population in any given 
        // low nibble or high nibble
        // even better there's the "right" algorithm, which bafflingly I don't know

        // easiest is just to limit to <8
        if (in.size() > 8) {
            throw std::logic_error("Cannot construct shuftis with >8 characters yet.");
        }
        u32 idx = 0; 
        for (u8 c : in) {
            u8 hi_nibble = c >> 4;
            u8 lo_nibble = c & 0x0f;
            set_bit(lo_nibble_mask, lo_nibble * 8 + idx);
            set_bit(hi_nibble_mask, hi_nibble * 8 + idx);
            idx++;
        }
        lo = _mm256_broadcastsi128_si256(lo_nibble_mask);
        hi = _mm256_broadcastsi128_si256(hi_nibble_mask);
    }
    
    u32 shufti_op(m256 input) {
        m256 t1 = _mm256_shuffle_epi8(lo, _mm256_and_si256(_mm256_set1_epi8(0x7f), input));
        m256 t2 = _mm256_shuffle_epi8(hi, _mm256_and_si256(_mm256_srli_epi32(input, 4), _mm256_set1_epi8(0x0f)));
        m256 t3 = _mm256_and_si256(t1, t2);
        m256 t4 = _mm256_cmpeq_epi8(t3, _mm256_set1_epi8(0));
        return ~(u32)_mm256_movemask_epi8(t4);
    }

    void scan(InputBlock input, Result<ResultType> & out) {
        apply_scanner_op<Shufti, &Shufti::shufti_op>(*this, input, out);
    }
};

