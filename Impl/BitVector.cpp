#include "PCH.h"

#include "WordRAM.h"
#include "BitVector.h"

class BitVector::_SelectBlockIndex {
	static constexpr Int _c = 4;
	bool _isTree;

	WordRAM::CompressedArray *_buf = nullptr;

public:
	_SelectBlockIndex(Int begin, Int end, BitVector *bv, bool bit) {
		Int lgn = lgceil(bv->_n);
		Int bound = (Int)pow(lgn, _c);
		Int length = end - begin + 1;
		Int wl = lgceil(length);
		if (length >= bound) {
			_isTree = false;
			_buf = new WordRAM::CompressedArray(length, wl);
			Int bi = 0;
			for (Int i = begin; i <= end; ++i) {
				if (bv->getBit(i) == bit) {
					_buf->set(bi, i - begin);
					bi++;
				}
			}
		} else {
			_isTree = true;
		}
	}

	~_SelectBlockIndex() {
		delete _buf;
	}

	Int select(Int i) {
		if (_isTree) {

		} else {
			return _buf->get(i) + 1;
		}
	}

	Int totalSize() {
		Int size = sizeof(*this);
		if (_isTree) {
			
		} else {
			size += _buf->totalSize();
		}
		return size;
	}
};

Int BitVector::_getMultiBits(Int begin, Int end) {
	assert(end < _n);
	Int wordLength = end - begin + 1;
	auto beginIdx = begin / CPU_WORD_LENGTH, endIdx = end / CPU_WORD_LENGTH;
	if (beginIdx == endIdx) {
		// The item is contained by one slot
		begin %= CPU_WORD_LENGTH;
		Int r = _d[beginIdx];
		r <<= begin;
		r >>= (CPU_WORD_LENGTH - wordLength);
		return r;
	} else {
		assert(beginIdx + 1 == endIdx);
		// The item is located in two adjacent slots
		begin %= CPU_WORD_LENGTH; end %= CPU_WORD_LENGTH;
		Int r0 = _d[beginIdx], r1 = _d[endIdx];
		r0 <<= begin; r0 >>= (CPU_WORD_LENGTH - wordLength);
		r1 >>= (CPU_WORD_LENGTH - end - 1);
		return r0 | r1;
	}
}

void BitVector::_buildIndexBF() {
	_r1BF = (Int *)malloc(_n * sizeof(Int));
	Int crt = 0;
	for (int i = 0; i < _n; ++i) {
		crt += getBit(i) == 1;
		_r1BF[i] = crt;
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

void BitVector::_buildIndexCA() {
	Int lgn = lgceil(_n);

	// Build rank index
	_rwl[1] = lgn; // at most n ones
	_rwl[2] = lgceil(_bs[1] + 1); // at most bs1 ones
	_rwl[3] = lgceil(_bs[2] + 1); // at most bs2 ones
	if (_verbose) {
		std::cout << "rwl: " << _rwl[1] << " " << _rwl[2] << " " << _rwl[3] << std::endl;
	}
	_rsize[1] = divceil(_n, _bs[1]);
	_rsize[2] = _rsize[1] * divceil(_bs[1], _bs[2]);
	_rsize[3] = (Int(1) << _bs[2]) * _bs[2]; // at most 2 * *`bs[2]` rows and`bs[2]` columns
	if (_verbose) {
		std::cout << "rsize: " << _rsize[1] << " " << _rsize[2] << " " << _rsize[3] << std::endl;
	}

	_r1CA = new WordRAM::CompressedArray(_rsize[1], _rwl[1]);
	_r2CA = new WordRAM::CompressedArray(_rsize[2], _rwl[2]);
	_r3CA = new WordRAM::CompressedArray(_rsize[3], _rwl[3]);

	Int c1 = 0; // c1 ones in the whole vector
	Int c2 = 0; // c2 ones in some large block
	Int i = 0; // i - th bit in the whole vector
	Int j = 0; // j - th bit in some large block
	Int k = 0; // k - th small block

	for (int i = 0; i < _n; ++i) {
		if (i % _bs[1] == 0) {
			_r1CA->set(i / _bs[1], c1);
			c2 = 0;
			j = 0;
		}
		if (j % _bs[2] == 0) {
			_r2CA->set(k, c2);
			k++;
		}

		auto b = getBit(i);
		if (b == 1) { // compute index for rank1
			c1++;
			c2++;
		}
		j++;
	}

	Int row = Int(1) << _bs[2], column = _bs[2]; // r&c for r3 table
	for (Int i = 0; i < row; ++i) {
		for (Int j = 0; j < column; ++j) {
			_r3CA->set(i * column + j, getOnes(i >> (column - j - 1)));
		}
	}
	if (_verbose) {
		std::cout << "r1: ";  _r1CA->print();
		std::cout << "r2: "; _r2CA->print();
		std::cout << "r3: "; _r3CA->print();
	}

	// Build index for select
	_n1 = rank1(_n - 1);
	_n0 = _n - _n1;

	Int swl1[2];
	swl1[0] = lgceil(_n0);
	swl1[1] = lgceil(_n1);
	_ssize1[0] = divceil(_n0, _bs[1]);
	_ssize1[1] = divceil(_n1, _bs[1]);

	_s1CA[0] = new WordRAM::CompressedArray(_ssize1[0], swl1[0]);
	_s1CA[1] = new WordRAM::CompressedArray(_ssize1[1], swl1[1]);
	_s2CA[0] = (_SelectBlockIndex *)malloc(_ssize1[0] * sizeof(_SelectBlockIndex));
	_s2CA[1] = (_SelectBlockIndex *)malloc(_ssize1[1] * sizeof(_SelectBlockIndex));

	Int cz = 0, co = 0; // currently we have `cz` zeros and `co` ones
	Int bz = 0, bo = 0; // currently we have `bz` blocks of zeros and `bo` blocks of ones
	Int lz = 0, lo = 0; // the beginging index of the current block

	for (Int i = 0; i < _n; ++i) {
		auto b = getBit(i);
		if (b == 0) {
			cz++;
		} else {
			co++;
		}

		if (cz == _bs[1]) {
			_s1CA[0]->set(bz, i);
			cz = 0;
			_s2CA[0][bz] = _SelectBlockIndex(lz, i, this, 0);
			bz++;
			lz = i + 1;
		}
		if (co == _bs[1]) {
			_s1CA[1]->set(bo, i);
			co = 0;
			_s2CA[1][bo] = _SelectBlockIndex(lo, i, this, 1);
			bo++;
			lo = i + 1;
		}
	}
	// build the last block
	if (lz != _n) {
		_s2CA[0][bz] = _SelectBlockIndex(lz, _n - 1, this, 0);
	}
	if (lo != _n) {
		_s2CA[1][bo] = _SelectBlockIndex(lo, _n - 1, this, 1);
	}
}

Int BitVector::_rank1BF(Int i) {
	return _r1BF[i];
}

Int BitVector::_rank1CA(Int i) {
	Int bs1num = i / _bs[1];
	Int bs1start = bs1num * _bs[1];
	Int bs1end = bs1start + _bs[1];
	i -= bs1start;
	Int bs2num = i / _bs[2];
	Int bs2start = bs2num * _bs[2];
	Int bs2end = bs2start + _bs[2];
	Int begin = bs1start + bs2start, end = bs1start + bs2end;
	Int bs2v;
	if (end > _n) {
		bs2v = _getMultiBits(bs1start + bs2start, _n - 1);
		bs2v <<= (end - _n);
	} else {
		bs2v = _getMultiBits(bs1start + bs2start, bs1start + bs2end - 1);
	}
	Int bs2i = i - bs2start;

	Int rank = 0;
	rank += _r1CA->get(bs1num);
	rank += _r2CA->get(bs1num * (divceil(_bs[1], _bs[2])) + bs2num);
	rank += _r3CA->get(bs2v * _bs[2] + bs2i);
	return rank;
}

Int BitVector::_select0BF(Int i) {
	return _s0BF[i];
}

Int BitVector::_select0CA(Int i) {
	return _s1CA[0]->get(i / _bs[1]) + _s2CA[0][i / _bs[1]].select(i % _bs[1]);
}

Int BitVector::_select1BF(Int i) {
	return _s1BF[i];
}

Int BitVector::_select1CA(Int i) {
	return _s1CA[1]->get(i / _bs[1]) + _s2CA[1][i / _bs[1]].select(i % _bs[1]);
}

BitVector::BitVector(Int n, Type type, bool verbose) : _n(n), type(type), _verbose(verbose) {
	// The actual size of the memory to store the vector
	_dataSize = divceil(_n, CPU_WORD_LENGTH) * (CPU_WORD_LENGTH / 8);
	_d = (Int *)malloc(_dataSize);

	Int lgn = lgceil(_n);
	if (lgn * lgn >= _n) {
		assertError("Size of bit vector is too small.");
	}

	_bs[1] = lgn * lgn;
	_bs[2] = lgn / 2;
	if (_verbose) {
		std::cout << "bs: " << _bs[1] << " " << _bs[2] << std::endl;
	}
}

BitVector::~BitVector() {
	free(_r1BF); free(_s0BF); free(_s1BF);
	delete _r1CA; delete _r2CA; delete _r3CA;
	delete _s1CA[0]; delete _s1CA[1];
	free(_s2CA[0]); free(_s2CA[1]);
}

bool BitVector::getBit(Int i) {
	assert(i < _n);
	return (_d[i / CPU_WORD_LENGTH] >> (CPU_WORD_LENGTH - (i + 1) % CPU_WORD_LENGTH)) & 1;
}

void BitVector::setBit(Int i, bool x) {
	assert(i < _n);
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
	case Type::SuccinctWithCompressedArray:
		size += _r1CA->totalSize();
		size += _r2CA->totalSize();
		size += _r3CA->totalSize();
		size += _s1CA[0]->totalSize();
		size += _s1CA[1]->totalSize();
		for (int b = 0; b < 2; ++b) {
			for (int i = 0; i < _ssize1[b]; ++i) {
				size += _s2CA[b][i].totalSize();
			}
		}
		break;
	}
	return size;
}

void BitVector::Test() {
	BitVector bv(13719, Type::SuccinctWithCompressedArray);

	// Test getter and setter
	bv.randomize();
	for (int i = 0; i < bv._n; ++i) {
		if (bv.getBit(i)) {
			bv.setBit(i, 0);
		}
	}
	for (int i = 0; i < bv._n / CPU_WORD_LENGTH; ++i) {
		if (bv._d[i] != 0) {
			assertError("Test Bit Vector Failed!");
			return;
		}
	}

	// Test rank
	Int rankTestLength = 13759;
	BitVector bv1(rankTestLength, Type::BruteForce), bv2(rankTestLength, Type::SuccinctWithCompressedArray);
	bv1.randomize(); bv2.randomize();
	bv1.buildIndex(); bv2.buildIndex();
	for (int i = 0; i < rankTestLength; ++i) {
		if (bv1.rank1(i) != bv2.rank1(i)) {
			std::cout << bv1.rank1(i);
			std::cout << " ";
			std::cout << bv2.rank1(i);
			std::cout << std::endl;
			assertError("Rank1 Test Failed!");
		}
		if (bv1.rank0(i) != bv2.rank0(i)) {
			std::cout << bv1.rank0(i);
			std::cout << " ";
			std::cout << bv2.rank0(i);
			std::cout << std::endl;
			assertError("Rank0 Test Failed!");
		}
	}

	// Test Select
}

inline void BitVector::randomize() {
	auto rg = RandomGenerator("Randomize Bit Vectors");
	for (int i = 0; i < _dataSize / (CPU_WORD_LENGTH / 8); ++i) {
		_d[i] = rg();
	}
}

void BitVector::buildIndex() {
	switch (type) {
	case Type::BruteForce:
		_buildIndexBF();
		break;
	case Type::SuccinctWithCompressedArray:
		_buildIndexCA();
		break;
	}
}

Int BitVector::rank0(Int i) {
	return i + 1 - rank1(i);
}

Int BitVector::rank1(Int i) {
	if (i >= _n) {
		throw std::out_of_range("Rank of " + std::to_string(i) + "-th bit when there are only " + std::to_string(_n) + " bits in the vector.");
	}
	switch (type) {
	case Type::BruteForce:
		return _rank1BF(i);
		break;
	case Type::SuccinctWithCompressedArray:
		return _rank1CA(i);
		break;
	default:
		fatalError();
	}
}

Int BitVector::select0(Int i) {
	i--; // internal the select result is zero-based
	if (i >= _n0) {
		throw std::out_of_range("Select of " + std::to_string(i) + "-th `0` when there are only " + std::to_string(_n0) + " `0`s in the vector.");
	}
	switch (type) {
	case Type::BruteForce:
		return _select0BF(i);
		break;
	case Type::SuccinctWithCompressedArray:
		return _select0CA(i);
		break;
	default:
		fatalError();
	}
}

Int BitVector::select1(Int i) {
	i--; // internal the select result is zero-based
	if (i >= _n1) {
		throw std::out_of_range("Select of " + std::to_string(i) + "-th `1` when there are only " + std::to_string(_n1) + " `1`s in the vector.");
	}
	switch (type) {
	case Type::BruteForce:
		return _select1BF(i);
		break;
	case Type::SuccinctWithCompressedArray:
		return _select1CA(i);
		break;
	default:
		fatalError();
	}
}
