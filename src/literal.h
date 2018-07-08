#pragma once

#include <string>
#include <utility>
#include "common_defs.h"
#include "scans.h"
#include "compare.h"

struct Literal {
	u32 id;
	std::string s;
	bool caseless;
	
	// is literal caseless at a given character?
	bool caselessAt(size_t i) const {
		return caseless && isalpha(s[i]);
	}
	
	std::pair<u64, u64> toFinalCmpMsk() const {
		u64 cmp = 0, msk = 0;
		for (u32 i = 0; i < s.size() && i < 8; i++) {
			size_t string_loc = s.size() - 1 - i; // i chars from end
			u8 c = s[string_loc];
			((u8 *)&cmp)[7 - i] = c;
			((u8 *)&msk)[7 - i] = caselessAt(string_loc) ? 0xdf : 0xff;
		}
		return std::make_pair(cmp, msk);
	}

    // a bit of a crock. This approach to comparing literals bakes in our InputBlock format
    // but the advantage of it is that we have a simple way to get started with using literals
    // in this context

    inline bool compare_in(const InputBlock & input, size_t idx) const {
        size_t effective_start = input.hard_start ? input.start : 0;
        // if literal extends beyond the effective start of this block, continue
        if (effective_start + s.size() - 1 > idx) {
            return false;
        }
        const u8 * location = input.buf + idx - s.size() + 1;
        if (caseless) {
            return compareNoCase(location, (const u8 *)s.c_str(), s.size());
        } else {
            return compare(location, (const u8 *)s.c_str(), s.size());
        }
    }
};
