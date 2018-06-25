#pragma once

#include <string>
#include <utility>
#include "common_defs.h"
#include "compare.h"

struct Literal {
	u32 id;
	std::string s;
	bool caseless;
	
	// is literal caseless at a given character?
	bool caselessAt(size_t i) {
		return caseless && isalpha(s[i]);
	}
	
	std::pair<u64, u64> toFinalCmpMsk() {
		u64 cmp = 0, msk = 0;
		for (u32 i = 0; i < s.size() && i < 8; i++) {
			size_t string_loc = s.size() - 1 - i; // i chars from end
			u8 c = s[string_loc];
			((u8 *)&cmp)[7 - i] = c;
			((u8 *)&msk)[7 - i] = caselessAt(string_loc) ? 0xdf : 0xff;
		}
		return std::make_pair(cmp, msk);
	}

    // low performance compare for 'gold' version only
    bool cmp_at(const u8 * location) {
        if (caseless) {
            return compareNoCase(location, (const u8 *)s.c_str(), s.size());
        } else {
            return compare(location, (const u8 *)s.c_str(), s.size());
        }
    }
};
