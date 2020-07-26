#include "BitVector.h"

#include <iostream>
#include <random>

#include "math_helpers.h"

void BitVector::_randomize() {
	std::mt19937_64 rg;
	std::string seed_str = "Randomize Bit Vectors";
	std::seed_seq seed(seed_str.begin(), seed_str.end());
	rg.seed(seed);
	for (int i = 0; i < _dataSize / (CPU_WORD_LENGTH / 8); ++i) {
		_d[i] = rg();
	}
}

void BitVector::_buildIndexBF() {
	_r0BF = (Int *)malloc(_n * sizeof(Int));
	Int crt = 0;
	for (int i = 0; i < _n; ++i) {
		crt += getBit(i) == 0;
		_r0BF[i] = crt;
	}

	_s0BF = (Int *)malloc(_n * sizeof(Int));
	_s1BF = (Int *)malloc(_n * sizeof(Int));
	Int crt0 = 0, crt1 = 0;
	for (int i = 0; i < _n; ++i) {
		auto b = getBit(i);
		if (b == 0) {
			_s0BF[crt0++] = i;
		} else {
			_s1BF[crt1++] = i;
		}
	}
	_n0 = crt0;
	_n1 = crt1;
}

void BitVector::_buildIndexS() {}

void BitVector::_buildIndexCA() {}

Int BitVector::_rank0BF(Int i) {
	return Int();
}

Int BitVector::_rank1BF(Int i) {
	return Int();
}

Int BitVector::_select0BF(Int i) {
	return Int();
}

Int BitVector::_select1BF(Int i) {
	return Int();
}

BitVector::BitVector(Int n, Type type) : _n(n), type(type) {
	// The actual size of the memory to store the vector
	_dataSize = divceil(n, CPU_WORD_LENGTH) * (CPU_WORD_LENGTH / 8);
	_d = (Int *)malloc(_dataSize);
}

bool BitVector::getBit(Int i) {
	return (_d[i / CPU_WORD_LENGTH] >> (CPU_WORD_LENGTH - (i + 1) % CPU_WORD_LENGTH)) & 1;
}

void BitVector::setBit(Int i, bool x) {
	if (x) {
		_d[i / CPU_WORD_LENGTH] |= (Int(1) << (CPU_WORD_LENGTH - 1 - i % CPU_WORD_LENGTH));
	} else {
		_d[i / CPU_WORD_LENGTH] &= ~(Int(1) << (CPU_WORD_LENGTH - 1 - i % CPU_WORD_LENGTH));
	}
}

Int BitVector::totalSize() {
	Int size = _dataSize + sizeof(*this);
	switch (type) {
	case Type::BruteForce:
		size += 3 * sizeof(Int) * _n;
		break;
	}
	return size;
}

void BitVector::Test() {
	BitVector bv(13719, Type::SuccinctWithCompressedArray);

	// Test getter and setter
	bv._randomize();
	for (int i = 0; i < bv._n; ++i) {
		if (bv.getBit(i)) {
			bv.setBit(i, 0);
		}
	}
	for (int i = 0; i < bv._n / CPU_WORD_LENGTH; ++i) {
		if (bv._d[i] != 0) {
			std::cerr << "Test Bit Vector Failed!" << std::endl;
			assert(false);
			return;
		}
	}
}

void BitVector::buildIndex() {
	switch (type) {
	case Type::BruteForce:
		_buildIndexBF();
		break;
	case Type::Succinct:
		_buildIndexS();
		break;
	case Type::SuccinctWithCompressedArray:
		_buildIndexCA();
		break;
	}
}

void BitVector::clearIndex() {
	_n0 = _n1 = 0;
	free(_r0BF); free(_s0BF); free(_s1BF);
	_r0BF = _s0BF = _s1BF = nullptr;
}

Int BitVector::rank0(Int i) {
	switch (type) {
	case Type::BruteForce:
		_rank0BF(i);
		break;
	case Type::Succinct:
		_rank0S(i);
		break;
	case Type::SuccinctWithCompressedArray:
		_rank0CA(i);
		break;
	}
}

Int BitVector::rank1(Int i) {
	return i + 1 - rank0(i);
}

Int BitVector::select0(Int i) {
	switch (type) {
	case Type::BruteForce:
		_select0BF(i);
		break;
	case Type::Succinct:
		_select0S(i);
		break;
	case Type::SuccinctWithCompressedArray:
		_select0CA(i);
		break;
	}
}

Int BitVector::select1(Int i) {
	switch (type) {
	case Type::BruteForce:
		_select1BF(i);
		break;
	case Type::Succinct:
		_select1S(i);
		break;
	case Type::SuccinctWithCompressedArray:
		_select1CA(i);
		break;
	}
}
