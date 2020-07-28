#pragma once

#include <cmath>
#include <cassert>
#include <cstdint>

#define CPU_WORD_LENGTH 64

#if CPU_WORD_LENGTH == 64
typedef uint64_t Int;
#endif

inline Int lg(Int x) {
	return int(log2(double(x)));
}

inline Int lgceil(Int x) {
	if (x == 0) return 0;
	else return lg(x - 1) + 1;
}

inline Int divceil(Int x, Int y) {
	assert(y > 0);
	if (x == 0) return 0;
	else return (x - 1) / y + 1;
}

inline Int getOnes(Int x) {
	Int o = 0;
	while (x > 0) {
		o += x & 1;
		x >>= 1;
	}
	return o;
}