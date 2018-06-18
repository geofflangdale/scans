#pragma once

#include <set>
#include "common_defs.h"
#include "scans.h"

#include <iostream>

class CharsetGold {
    std::set<u8> s;
public:
    CharsetGold(const std::set<u8> & s_in) : s(s_in) {
    }

    void scan(InputBlock input, std::vector<u32> & out) {
        u32 result_idx = 0;
        u8 * buf = input.first;
        size_t len = input.second;
        for (size_t idx = 0; idx < len; idx+=32) {
            for (size_t j = idx; j < idx+32; j++) {
                if (s.find(buf[j]) != s.end()) {
                    out[result_idx++] = j;
                }
            }
        }
        out.resize(result_idx);
    }
};

