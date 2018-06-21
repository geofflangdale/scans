#pragma once

#include <vector>
#include <set>
#include <stdexcept>
#include "common_defs.h"
#include "util.h"
#include "scans.h"
#include "vermicelli.h"

class DVerm {
    Vermicelli sub[2];
    u32 distance;
public:
    DVerm(const DoubleCharsetWorkload & work) : sub({std::get<0>(work), std::get<1>(work)}) {
        distance = std::get<2>(work);
        if (distance == 0) {
            throw std::logic_error("Can't have distance == 0");
        }
        if (distance > 63) {
            throw std::logic_error("Can't have distance > 63");
        }
    }

    void scan(InputBlock input, std::vector<u32> & out, UNUSED std::vector<u8> & tmp) {
        apply_double_scanner_op<Vermicelli, &Vermicelli::vermicelli_op,
                                Vermicelli, &Vermicelli::vermicelli_op>(sub[0], sub[1], distance, input, out);
    }
};
