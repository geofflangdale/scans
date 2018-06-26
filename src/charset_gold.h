#pragma once

#include <set>
#include "common_defs.h"
#include "scans.h"

#include <iostream>

class CharsetGold {
    std::set<u8> s;
public:
    typedef OffsetResult ResultType;

    CharsetGold(const std::set<u8> & s_in) : s(s_in) {
    }

    void scan(InputBlock input, Result<ResultType> & out) {
        u32 result_idx = 0;
        for (size_t i = input.start; i < input.end; i++) {
            if (s.find(input.buf[i]) != s.end()) {
                out.results[result_idx++] = i;
            }
        }
        out.trim(result_idx);
    }
};

