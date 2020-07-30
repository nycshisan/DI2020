#pragma once

#include "helpers.h"

namespace WordRAM {
	class CompressedArray {
		Int _size, _wordLength, _arraySize;

		Int *_d = nullptr;

	public:
		CompressedArray(Int size, Int wordLength);

		~CompressedArray();

		void destroy();

		Int size();

		Int maxValue();

		Int totalSize();

		Int get(Int i);

		void set(Int i, Int x);

		void print();

		static void Test();
	};
}

