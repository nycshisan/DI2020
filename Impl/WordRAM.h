#pragma once

#include "helpers.h"

namespace WordRAM {
	class CompressedArray {
	protected:
		Int _size, _wordLength, _arraySize;

		Int *_d = nullptr;

	public:
		CompressedArray(Int size, Int wordLength);

		~CompressedArray();

		void destroy();

		Int size();

		Int maxValue();

		virtual Int totalSize();

		Int get(Int i);

		void set(Int i, Int x);

		void print();

		static void Test();
	};

	class CompressedArray2D : public CompressedArray {
		Int _column;

	public:
		CompressedArray2D(Int row, Int column, Int wordLength);

		virtual Int totalSize();

		Int get2D(Int i, Int j);

		void set2D(Int i, Int j, Int x);
	};
}

