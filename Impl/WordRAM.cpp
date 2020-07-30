#include "PCH.h"

#include "WordRAM.h"

#if CPU_WORD_LENGTH == 64
static constexpr Int DIVCWL = 6;
static constexpr Int MODCWL = 0x3f;
#endif

WordRAM::CompressedArray::CompressedArray(Int size, Int wordLength) : _size(size), _wordLength(wordLength) {
	assert(size > 0); assert(wordLength > 0);
	if (wordLength > CPU_WORD_LENGTH) {
		assertError("Error! Word length for compressed arrays should be less than the word length of processors.");
	}

	_arraySize = size * wordLength;
	_arraySize = divceil(_arraySize, CPU_WORD_LENGTH) * (CPU_WORD_LENGTH / 8); // The actual size that we will allocate

	_d = (Int *)malloc(_arraySize);
}

WordRAM::CompressedArray::~CompressedArray() {
	destroy();
}

void WordRAM::CompressedArray::destroy() {
	free(_d);
}

Int WordRAM::CompressedArray::totalSize() {
	return _arraySize + sizeof(*this);
}

Int WordRAM::CompressedArray::get(Int i) {
	assert(i < _size);
	auto begin = i * _wordLength, end = begin + _wordLength - 1;
	auto beginIdx = begin >> DIVCWL, endIdx = end >> DIVCWL;
	if (beginIdx == endIdx) {
		// The item is contained by one slot
		begin &= MODCWL;
		Int r = _d[beginIdx];
		r <<= begin;
		r >>= (CPU_WORD_LENGTH - _wordLength);
		return r;
	} else {
		assert(beginIdx + 1 == endIdx);
		// The item is located in two adjacent slots
		begin &= MODCWL; end &= MODCWL;
		Int r0 = _d[beginIdx], r1 = _d[endIdx];
		r0 <<= begin; r0 >>= (CPU_WORD_LENGTH - _wordLength);
		r1 >>= (CPU_WORD_LENGTH - end - 1);
		return r0 | r1;
	}
}

void WordRAM::CompressedArray::set(Int i, Int x) {
	assert(i < _size);
	assert(x < ((Int(1) << _wordLength)));
	auto begin = i * _wordLength, end = begin + _wordLength - 1;
	auto beginIdx = begin >> DIVCWL, endIdx = end >> DIVCWL;
	if (beginIdx == endIdx) {
		// The item is contained by one slot
		begin &= MODCWL;
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
		begin &= MODCWL; end &= MODCWL;
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

void WordRAM::CompressedArray::print() {
	for (int i = 0; i < _size; ++i) {
		std::cout << get(i) << " ";
	}
	std::cout << std::endl;
}

void WordRAM::CompressedArray::Test() {
	auto rg = RandomGenerator("Test Compressed Array");

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
			assertError("Test Compressed Array Failed!");
			return;
		}
	}
	for (int i = testSize - 1; i >= 0; --i) {
		ca.set(i, a[i]);
	}
	for (int i = testSize - 1; i >= 0; --i) {
		auto cav = ca.get(i);
		if (cav != a[i]) {
			assertError("Test Compressed Array Failed!");
			return;
		}
	}
}
