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
        for (size_t i = input.start; i < input.end; i++) {
            for (auto & l : lits) {
                if (l.compare_in(input, i)) {
                    out.results[result_idx++] = std::make_pair(i, l.id);
                }
            }
        }
        out.trim(result_idx);
    }
};

