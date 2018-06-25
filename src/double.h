#pragma once

#include <vector>
#include <set>
#include <stdexcept>
#include "common_defs.h"
#include "util.h"
#include "scans.h"

template<typename T0, u32 (T0::*op0)(m256 input),
         typename T1, u32 (T1::*op1)(m256 input)>
class DoubleMatcher {
    T0 s0;
    T1 s1;
    u32 distance;
public:
    DoubleMatcher(const DoubleCharsetWorkload & work) : s0(std::get<0>(work)), s1(std::get<1>(work)) {
        distance = std::get<2>(work);
        if (distance == 0) {
            throw std::logic_error("Can't have distance == 0");
        }
        if (distance > 63) {
            throw std::logic_error("Can't have distance > 63");
        }
    }

    void scan(InputBlock input, std::vector<u32> & out) {
        apply_double_scanner_op<T0, op0,
                                T1, op1>(s0, s1, distance, input, out);
    }
};


