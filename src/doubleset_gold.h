#pragma once

#include <vector>
#include <set>
#include <stdexcept>
#include "common_defs.h"
#include "util.h"
#include "scans.h"
#include "vermicelli.h"

class DoublesetGold {
    std::set<u8> s[2];
    u32 distance;
public:
    typedef OffsetResult ResultType;

    DoublesetGold(const DoubleCharsetWorkload & work) : s({std::get<0>(work), std::get<1>(work)}) {
        distance = std::get<2>(work);
        if (distance == 0) {
            throw std::logic_error("Can't have distance == 0");
        }
        if (distance > 63) {
            throw std::logic_error("Can't have distance > 63");
        }
    }

    void scan(InputBlock input, Result<ResultType> & out) {
        u32 result_idx = 0;
        for (size_t i = input.start; i < input.end; i++) {
            if (s[1].find(input.buf[i]) != s[1].end()) {
                if ((i >= distance) && (s[0].find(input.buf[i - distance]) != s[0].end())) {
                    out.results[result_idx++] = i;
                }
            }
        }
        out.trim(result_idx);
    }
};
