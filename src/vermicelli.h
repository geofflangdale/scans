#pragma once

#include <vector>
#include <set>
#include <x86intrin.h>
#include "common_defs.h"
#include "util.h"
#include "scans.h"

#include <iostream>

class Vermicelli {
    m256 and_mask;
    m256 cmp_mask;
public:
    Vermicelli(const std::set<u8> & in) {
        if (in.size() > 2) {
            throw "We don't support this yet";
        }

        u8 cmp, msk = 0xff;

        if (in.size() == 2) {
            auto i = in.begin();
            u8 c1 = *i++;
            u8 c2 = *i++;
            if (popcount(c1 ^ c2) != 1) {
                throw "Vermicelli only supports size 2 sets differing in 1 bit";
            }
            msk = ~(c1 ^ c2);
            cmp = c1 & msk;
        } else {
            cmp = *(in.begin());
            msk = 0xff;
        }
        and_mask = _mm256_set1_epi8(msk);
        cmp_mask = _mm256_set1_epi8(cmp);
    }
    
    u32 vermicelli_op(m256 input) {
        m256 t = _mm256_cmpeq_epi8(_mm256_and_si256(input, and_mask), cmp_mask);
        return (u32)_mm256_movemask_epi8(t);
    }

    void scan(InputBlock input, std::vector<u32> & out, UNUSED std::vector<u8> & tmp) {
        apply_scanner_op<Vermicelli, &Vermicelli::vermicelli_op>(*this, input, out);
    }
};

