#pragma once

#include "math_helpers.h"
#include "WordRAM.h"

class BitVector {
	Int _n, _dataSize;

	Int *_d;

	// Index
	Int _n0 = 0, _n1 = 0;
	// Index for brute force approach
	Int *_r0BF = nullptr, *_s0BF = nullptr, *_s1BF = nullptr;

	void _randomize();

	void _buildIndexBF(); // brute force
	void _buildIndexS(); // succinct
	void _buildIndexCA(); // compressed array

	Int _rank0BF(Int i); // brute force
	Int _rank0S(Int i); // succinct
	Int _rank0CA(Int i); // compressed array
	Int _rank1BF(Int i); // brute force
	Int _rank1S(Int i); // succinct
	Int _rank1CA(Int i); // compressed array

	Int _select0BF(Int i); // brute force
	Int _select0S(Int i); // succinct
	Int _select0CA(Int i); // compressed array
	Int _select1BF(Int i); // brute force
	Int _select1S(Int i); // succinct
	Int _select1CA(Int i); // compressed array

public:
	enum class Type {
		BruteForce,
		Succinct,
		SuccinctWithCompressedArray
	};

	Type type;

	BitVector(Int n, Type type);

	bool getBit(Int i);

	void setBit(Int i, bool x);

	Int totalSize();

	static void Test();

	void buildIndex();
	void clearIndex();

	Int rank0(Int i);
	Int rank1(Int i);
	Int select0(Int i);
	Int select1(Int i);
};

