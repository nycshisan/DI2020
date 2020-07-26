#include "WordRAM.h"

#include <iostream>
#include <cassert>
#include <random>

WordRAM::CompressedArray::CompressedArray(Int size, Int wordLength) : _size(size), _wordLength(wordLength) {
	assert(size > 0); assert(wordLength > 0);
	if (wordLength > CPU_WORD_LENGTH) {
		std::cerr << "Error! Word length for compressed arrays should be less than the word length of processors." << std::endl;
	}

	_arraySize = size * wordLength;
	_arraySize = divceil(_arraySize, CPU_WORD_LENGTH) * (CPU_WORD_LENGTH / 8); // The actual size that we will allocate

	_d = (Int *)malloc(_arraySize);
}

WordRAM::CompressedArray::~CompressedArray() {
	free(_d);
}

size_t WordRAM::CompressedArray::totalSize() {
	return _arraySize + sizeof(*this);
}

Int WordRAM::CompressedArray::get(Int i) {
	auto begin = i * _wordLength, end = begin + _wordLength - 1;
	auto beginIdx = begin / CPU_WORD_LENGTH, endIdx = end / CPU_WORD_LENGTH;
	if (beginIdx == endIdx) {
		// The item is contained by one slot
		begin %= CPU_WORD_LENGTH;
		Int r = _d[beginIdx];
		r <<= begin;
		r >>= (CPU_WORD_LENGTH - _wordLength);
		return r;
	} else {
		assert(beginIdx + 1 == endIdx);
		// The item is located in two adjacent slots
		begin %= CPU_WORD_LENGTH; end %= CPU_WORD_LENGTH;
		Int r0 = _d[beginIdx], r1 = _d[endIdx];
		r0 <<= begin; r0 >>= (CPU_WORD_LENGTH - _wordLength);
		r1 >>= (CPU_WORD_LENGTH - end - 1);
		return r0 | r1;
	}
}

void WordRAM::CompressedArray::set(Int i, Int x) {
	auto begin = i * _wordLength, end = begin + _wordLength - 1;
	auto beginIdx = begin / CPU_WORD_LENGTH, endIdx = end / CPU_WORD_LENGTH;
	if (beginIdx == endIdx) {
		// The item is contained by one slot
		begin %= CPU_WORD_LENGTH;
		// Clear the bits
		Int mask = 0; mask = ~mask;
		mask <<= (CPU_WORD_LENGTH - _wordLength); mask >>= begin;
		mask = ~mask;
		_d[beginIdx] &= mask;
		// Set the value
		x <<= (CPU_WORD_LENGTH - begin - _wordLength);
		_d[beginIdx] |= x;
	} else {
		assert(beginIdx + 1 == endIdx);
		// The item is located in two adjacent slots
		begin %= CPU_WORD_LENGTH; end %= CPU_WORD_LENGTH;
		// Calc x0 and x1
		Int x0 = x >> (end + 1);
		Int x1 = x << (CPU_WORD_LENGTH - end - 1);
		// Clear the bits
		Int mask0 = 0; mask0 = ~mask0;
		mask0 <<= (CPU_WORD_LENGTH - begin);
		_d[beginIdx] &= mask0;
		Int mask1 = 0; mask1 = ~mask1;
		mask1 >>= (end + 1);
		_d[endIdx] &= mask1;
		// Set the values
		_d[beginIdx] |= x0;
		_d[endIdx] |= x1;
	}
}

void WordRAM::CompressedArray::Test() {
	std::mt19937_64 rg;
	std::string seed_str = "Test Compressed Array";
	std::seed_seq seed(seed_str.begin(), seed_str.end());
	rg.seed(seed);

	Int constexpr testSize = 137, testWordLength = 19;
	Int constexpr maxValue = (Int(1) << testWordLength) - Int(1);

	Int a[testSize];

	for (int i = 0; i < testSize; ++i) {
		a[i] = rg() % maxValue;
	}

	CompressedArray ca(testSize, testWordLength);
	for (int i = 0; i < testSize; ++i) {
		ca.set(i, a[i]);
	}
	for (int i = 0; i < testSize; ++i) {
		auto cav = ca.get(i);
		if (cav != a[i]) {
			std::cerr << "Test Compressed Array Failed!" << std::endl;
			assert(false);
			return;
		}
	}
	for (int i = testSize - 1; i >= 0; --i) {
		ca.set(i, a[i]);
	}
	for (int i = testSize - 1; i >= 0; --i) {
		auto cav = ca.get(i);
		if (cav != a[i]) {
			std::cerr << "Test Compressed Array Failed!" << std::endl;
			assert(false);
			return;
		}
	}
}
