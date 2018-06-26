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
        u32 n_lits = static_cast<u32>(lits.size());
        size_t effective_start = input.hard_start ? input.start : 0;
        for (size_t i = input.start; i < input.end; i++) {
            for (u32 j = 0; j < n_lits; j++) {
                Literal & l = lits[j];
                // if literal extends beyond the effective start of this block, continue
                if (effective_start + l.s.size() - 1 > i) {
                    continue;
                }
                if (l.cmp_at(input.buf + i - l.s.size() + 1)) {
                    out.results[result_idx++] = std::make_pair(i, l.id);
                }
            }
        }
        out.trim(result_idx);
    }
};

