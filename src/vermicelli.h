#pragma once

#include <vector>
#include <set>
#include <x86intrin.h>
#include <stdexcept>
#include "common_defs.h"
#include "util.h"
#include "scans.h"

#include <iostream>


class Vermicelli {
    m256 and_mask;
    m256 cmp_mask;
public:
    typedef OffsetResult ResultType;

    Vermicelli(const std::set<u8> & in) {
        
        if (in.size() > 2) {
            throw std::logic_error("Vermicelli only supports 1-2 characters for now.");
        }

        u8 cmp, msk = 0xff;

        if (in.size() == 2) {
            auto i = in.begin();
            u8 c1 = *i++;
            u8 c2 = *i++;
            if (popcount(c1 ^ c2) != 1) {
                throw std::logic_error("Vermicelli can only support size 2 sets differing only at 1 bit.");
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

    void scan(InputBlock input, Result<ResultType> & out) {
        apply_scanner_op<Vermicelli, &Vermicelli::vermicelli_op>(*this, input, out);
    }
};
