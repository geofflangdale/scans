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
        u8 * buf = input.first;
        size_t len = input.second;
        for (size_t idx = 0; idx < len; idx+=32) {
            for (size_t j = idx; j < idx+32; j++) {
                if (s[1].find(buf[j]) != s[1].end()) {
                    if ((j >= distance) && (s[0].find(buf[j - distance]) != s[0].end())) {
                        out.results[result_idx++] = j;
                    }
                }
            }
        }
        out.trim(result_idx);
    }
};
