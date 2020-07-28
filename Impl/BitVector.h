#pragma once

#include "helpers.h"

#include "WordRAM.h"

class BitVector {
	class _SelectBlockIndex;

	Int _n, _dataSize;

	Int *_d;

	bool _verbose;

	// Index
	Int _n0 = 0, _n1 = 0;
	// Index for brute force approach
	Int *_r1BF = nullptr, *_s0BF = nullptr, *_s1BF = nullptr;
	// Index for compressed array approach
	Int _bs[3] = {}; // block sizes, allocate one more slot for better readability
	Int _rwl[4] = {}, _rsize[4] = {}; // word lengths and sizes of rank index, allocate one more slot for better readability
	WordRAM::CompressedArray *_r1CA = nullptr, *_r2CA = nullptr, *_r3CA = nullptr;

	WordRAM::CompressedArray *_s1CA[2] = {};
	Int _ssize1[2] = {}; // block number of S1 index
	_SelectBlockIndex *_s2CA[2] = {};

	Int _getMultiBits(Int begin, Int end);

	void _buildIndexBF(); // brute force
	void _buildIndexCA(); // compressed array

	Int _rank1BF(Int i); // brute force
	Int _rank1CA(Int i); // compressed array

	Int _select0BF(Int i); // brute force
	Int _select0CA(Int i); // compressed array
	Int _select1BF(Int i); // brute force
	Int _select1CA(Int i); // compressed array

public:
	enum class Type {
		BruteForce,
		SuccinctWithCompressedArray
	};

	Type type;

	BitVector(Int n, Type type, bool verbose = false);

	~BitVector();

	bool getBit(Int i);

	void setBit(Int i, bool x);

	Int totalSize();

	static void Test();

	void randomize();

	void buildIndex();

	Int rank0(Int i);
	Int rank1(Int i);
	Int select0(Int i);
	Int select1(Int i);
};

