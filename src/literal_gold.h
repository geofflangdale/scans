#pragma once

#include <set>
#include "common_defs.h"
#include "scans.h"
#include "literal.h"
#include "compare.h"

#include <iostream>

class LiteralGold {
    std::vector<Literal> lits;
public:
    typedef OffsetIDResult ResultType;

    LiteralGold(const std::vector<Literal> & lits_in) : lits(lits_in) {
    }

    void scan(InputBlock input, Result<ResultType> & out) {
        u32 result_idx = 0;
        u8 * buf = input.first;
        size_t len = input.second;

        u32 n_lits = static_cast<u32>(lits.size());

        for (size_t idx = 0; idx < len; idx+=32) {
            for (size_t j = idx; j < idx+32; j++) {
                for (u32 k = 0; k < n_lits; k++) {
                    // if literal too long, continue
                    Literal & l = lits[k];
                    if (j + l.s.size() - 1 > len) {
                        continue;
                    }
                    if (l.cmp_at(buf + j - l.s.size() + 1)) {
                        out.results[result_idx++] = std::make_pair(j, l.id);
                    }
                }

            }
        }
        out.trim(result_idx);
    }
};

