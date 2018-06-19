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

    void scan(InputBlock input, std::vector<u32> & out, UNUSED std::vector<u8> & tmp) {
        u8 * buf = input.first;
        size_t len = input.second;
        dump256(lo, "lo");
        dump256(hi, "hi");
        u32 result_idx = 0;
        for (size_t idx = 0; idx < len; idx+=64) {
            __builtin_prefetch(buf+ idx + 32*128);
            m256 input_0 = _mm256_load_si256((const m256 *)(buf + idx));
            m256 input_1 = _mm256_load_si256((const m256 *)(buf + idx + 32));
            m256 t1_0 = _mm256_cmpeq_epi8(_mm256_and_si256(input_0, and_mask), cmp_mask);
            m256 t1_1 = _mm256_cmpeq_epi8(_mm256_and_si256(input_1, and_mask), cmp_mask);
            dump256(t1_0, "t1_0");
            dump256(t1_1, "t1_1");
            u64 res_0 = (u32)_mm256_movemask_epi8(t1_0);
            u64 res_1 = (u64)_mm256_movemask_epi8(t1_1) << 32;
            u64 res = res_0 | res_1;
            while (res) {
                out[result_idx++] = (u32)idx + __builtin_ctzll(res);
                res &= res - 1ULL;
            }
        }
        out.resize(result_idx);
    }
};

