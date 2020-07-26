#pragma once

#include "math_helpers.h"

namespace WordRAM {
	class CompressedArray {
		Int _size, _wordLength, _arraySize;

		Int *_d;

	public:
		CompressedArray(Int size, Int wordLength);

		~CompressedArray();

		size_t totalSize();

		Int get(Int i);

		void set(Int i, Int x);

		static void Test();
	};
}

