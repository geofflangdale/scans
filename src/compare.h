#pragma once

#include "common_defs.h"

// comparison utilities
// by convention our dynamic buffer is in p1 and our own bytecode is in p2
// holding the convention that it is not illegal to read ANYWHERE plus or minus a IA cache line size from p2. That is, our 
// bytecodes always have a 64-byte buffer before and after the actual bytecode
// holding the convention that it is not illegal to read anywhere on a cache line that any address in p1 occupies. That is, it
// is not permissible to pass us a buffer with something that goes 'boom' if we read somewhere else on the cache line. We must
// not, however, base any actions on the contents of buffer (should be valgrind-safe)
// p2 should probably not span cache lines if that can be avoided (again, this is a convention)

inline u8 mytoupper8(const u8 x) {
    if (x >= 'a' && x <= 'z') {
        return x & 0xdf;
    } else  {
        return x;
    }
}
// from HS
inline u64 theirtoupper64(const u64 x) {
	u64 b = 0x8080808080808080ull | x;
	u64 c = b - 0x6161616161616161ull;
	u64 d = ~(b - 0x7b7b7b7b7b7b7b7bull);
	u64 e = (c & d) & (~x & 0x8080808080808080ull);
	u64 v = x - (e >> 2);
	return v;
}

const int COMPARE_SIZE = 8;

inline bool compare(const u8 * p1, const u8 * p2, size_t len) {
	for (u32 i = 0; i < len; i++) {
		if (p1[i] != p2[i]) {
			return false;
		}
	}
	return true;
}

// a further convention - p2 has already been 'touppered'
inline bool compareNoCase(const u8 * p1, const u8 * p2, size_t len) {
	for (u32 i = 0; i < len; i++) {
		if (mytoupper8(p1[i]) != p2[i]) {
			return false;
		}
	}
	return true;
}
