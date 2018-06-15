#pragma once

#include <vector>
#include <set>
#include <x86intrin.h>
#include "common_defs.h"
#include "scans.h"

#include <iostream>

class Truffle {
    m256 lo_set_mask = _mm256_set1_epi8(0);
    m256 hi_set_mask = _mm256_set1_epi8(0);
    
    // set a bit in our masks for both lo/high 128-bit masks
    inline void set_bit_mirrored(bool high, u32 bit) {
        m256 & mask = high ? hi_set_mask : lo_set_mask;
        union {
            m256 avx;
            u8 bytes[32];
        } u;
        u.avx = mask;
        u.bytes[bit/8     ] |= (1 << bit%8);
        u.bytes[bit/8 + 16] |= (1 << bit%8);
        mask = u.avx;
    }

public:
    Truffle(std::set<u8> & inputs) {
        for (auto c : inputs) {
            bool high = c & 0x80;
            u32 byte = c & 0x0f;
            u32 bit = (c & 0x70) >> 4;
            set_bit_mirrored(high, byte*8 + bit);
        }
    }

    void scan(InputBlock input, std::vector<u32> & out) {
        const m256 log2_mask = _mm256_setr_epi8(
            1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128,
            1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128
        );
        u8 * buf = input.first;
        size_t len = input.second;
        dump256(log2_mask, "log2_mask");
        dump256(lo_set_mask, "lo_set_mask");
        dump256(hi_set_mask, "hi_set_mask");
        u32 result_idx = 0;
        for (size_t idx = 0; idx < len; idx+=32) {
            m256 input = _mm256_load_si256((const m256 *)(buf + idx));
            m256 t1 = _mm256_shuffle_epi8(lo_set_mask, input);
            dump256(t1, "t1");
            m256 t2 = _mm256_shuffle_epi8(hi_set_mask, _mm256_xor_si256(input, _mm256_set1_epi8(0x80)));
            dump256(t2, "t2");
            m256 t3 = _mm256_or_si256(t1, t2);
            dump256(t3, "t3");
            m256 t4 = _mm256_and_si256(t3, _mm256_shuffle_epi8(
                                                log2_mask, 
                                                _mm256_and_si256(_mm256_srli_epi32(input, 4),
                                                                 _mm256_set1_epi8(0x07))));
            dump256(t4, "t4");
            m256 t5 = _mm256_cmpeq_epi8(t4, _mm256_set1_epi8(0));
            dump256(t4, "t5");
            u32 res = ~(u32)_mm256_movemask_epi8(t5);
            while (res) {
                //std::cout << " recording " << ((u32)idx + __builtin_ctz(res)) << "\n";
                out[result_idx++] = (u32)idx + __builtin_ctz(res);
                //out.push_back((u32)idx + __builtin_ctz(res));
                res &= res - 1ULL;
            }
        }
        out.resize(result_idx);
    }
};

