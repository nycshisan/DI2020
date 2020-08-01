#include "PCH.h"

#include "WordRAM.h"
#include "BitVector.h"

class BitVector::_SelectTreeTable {
public:
	class Item {
		WordRAM::CompressedArray *_table, *_left;
		std::vector<Int> _cbuf;
	public:
		Item(const std::vector<Int> &cbuf) {
			_cbuf = cbuf;
			if (cbuf.size() == 1 && cbuf[0] == 1) {
				std::cout << "fuck";
			}
			Int csum = 0;
			for (int i = 0; i < cbuf.size(); ++i) {
				csum += cbuf[i];
			}
			Int wl = lgceil(cbuf.size());
			_table = new WordRAM::CompressedArray(csum, wl);
			Int crt = 0; // current idx since last child
			Int cid = 0; // current child idx
			for (int i = 0; i < csum; ++i) {
				while (cid < cbuf.size() && crt == cbuf[cid]) {
					crt = 0;
					cid++;
				}
				_table->set(i, cid);
				crt++;
			}
			_left = new WordRAM::CompressedArray(cbuf.size(), lgceil(csum + 1));
			Int leftCount = 0;
			for (int i = 0; i < cbuf.size(); ++i) {
				_left->set(i, leftCount);
				leftCount += cbuf[i];
			}
		}

		~Item() {
			destroy();
		}

		void destroy() {
			delete _table;
			delete _left;
		}

		Int totalSize() {
			return sizeof(*this) + _table->totalSize() + _left->totalSize();
		}

		Int getChildIdx(Int i) {
			return _table->get(i);
		}

		Int getLeftChildrenNumber(Int i) {
			return _left->get(i);
		}
	};
private:
	Int _bs;
	std::unordered_map<Int, Item *> _map;
	Item **_initedItems = nullptr;
	Int _itemNum = 0;

public:
	_SelectTreeTable(Int bs) : _bs(bs) {}

	~_SelectTreeTable() {
		for (int i = 0; i < _itemNum; ++i) {
			delete _initedItems[i];
		}
		delete _initedItems;
	}

	void finishInit() {
		_itemNum = _map.size();
		_initedItems = new Item *[_itemNum];
		auto it = _map.begin();
		for (int i = 0; i < _itemNum; ++i) {
			_initedItems[i] = it->second;
			it++;
		}
	}

	Int totalSize() {
		Int size = sizeof(*this);
		size += _itemNum * sizeof(Item *);
		for (int i = 0; i < _itemNum; ++i) {
			size += _initedItems[i]->totalSize();
		}
		return size;
	}

	Item *getItem(const std::vector<Int> &cbuf) {
		Int id = 0;
		for (int i = 0; i < cbuf.size(); ++i) {
			assert(id < std::numeric_limits<Int>::max() / _bs - 1);
			id *= _bs; id += cbuf[i];
		}
		if (_map[id] == nullptr) {
			_map[id] = new Item(cbuf);
		}
		return _map[id];
	}
};

class BitVector::_SelectBlockIndex {
	static constexpr Int _c = 4;

	BitVector *_bv;

	bool _isTree;
	Int _h = 0, _cn = 0, _bs = 0;
	Int _begin;

	WordRAM::CompressedArray *_buf = nullptr;

	_SelectTreeTable::Item ***_items = nullptr;
	Int _itemNum = 0;

public:
	_SelectBlockIndex(Int begin, Int end, BitVector *bv, bool bit, _SelectTreeTable &s3table) {
		_bv = bv;
		_begin = begin;

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
			_cn = Int(sqrt(lgn)); // children number of the tree nodes
			_bs = lgn / 2; // small block size
			Int wl = lgceil(lgn * lgn); // word length of the tree
			Int ln = divceil(length, _bs); // number of leaves
			_h = lgceil(ln, _cn) + 1;

			_items = new _SelectTreeTable::Item **[_h - 1];
			
			_buf = (WordRAM::CompressedArray *)malloc(_h * sizeof(WordRAM::CompressedArray)); // rows of the tree
			// First create leaves
			new (&_buf[_h - 1]) WordRAM::CompressedArray(ln, wl);
			Int bid = 0; // current small block index
			Int bc = 0; // bit counter
			for (Int i = begin; i <= end; ++i) {
				if (bv->getBit(i) == bit) {
					bc++;
				}
				if ((i - begin + 1) % _bs == 0) {
					_buf[_h - 1].set(bid, bc);
					bid++;
					bc = 0;
				}
			}
			if (length % _bs != 0) {
				_buf[_h - 1].set(bid, bc); // the last block
				bid++;
			}
			assert(bid == ln);
			std::vector<Int> cbuf; // children buffer for getting s3 item
			// Build non-leaf nodes
			for (int r = int(_h - 2); r >= 0; --r) {
				cbuf.clear();
				Int w = divceil(_buf[r + 1].size(), _cn); // current row width
				_items[r] = new _SelectTreeTable::Item * [w];
				_itemNum += w;
				new (&_buf[r]) WordRAM::CompressedArray(w, wl);
				bid = bc = 0;
				for (Int i = 0; i < _buf[r + 1].size(); ++i) {
					auto c = _buf[r + 1].get(i);
					bc += c;
					cbuf.emplace_back(c);
					if ((i + 1) % _cn == 0) {
						_buf[r].set(bid, bc);
						_items[r][bid] = s3table.getItem(cbuf);
						bid++;
						bc = 0;
						cbuf.clear();
					}
				}
				if (_buf[r + 1].size() % _cn != 0) {
					_buf[r].set(bid, bc); // the last block
					while (cbuf.size() < _cn) {
						cbuf.emplace_back(0);
					}
					_items[r][bid] = s3table.getItem(cbuf);
					bid++;
				}
				assert(bid == w);
				if (r == 0) {
					assert(_buf[r].size() == 1);
				} else {
					assert(_buf[r].size() > 1);
				}
			}
		}
	}

	~_SelectBlockIndex() {
		destroy();
	}

	void destroy() {
		if (_isTree) {
			for (int i = 0; i < _h; ++i) {
				_buf[i].destroy();
			}
			free(_buf);
			for (int i = 0; i < _h - 1; ++i) {
				delete[] _items[i];
			}
			delete[] _items;
		} else {
			delete _buf;
		}
	}

	Int select(Int i, bool bit) {
		if (_isTree) {
			Int bid = 0; // small block idx
			for (int r = 0; r < _h - 1; ++r) {
				auto item = _items[r][bid];
				Int cid = item->getChildIdx(i);
				bid = bid * _cn + cid;
				i -= item->getLeftChildrenNumber(cid);
			}
			Int begin = _begin + bid * _bs; // begin of the target small block
			Int end = begin + _bs;
			Int v;
			if (end <= _bv->_n) {
				v = _bv->_getMultiBits(begin, begin + _bs - 1);
			} else {
				v = _bv->_getMultiBits(begin, _bv->_n - 1);
				v <<= (end - _bv->_n);
			}
			Int io = bid * _bs, iob = _bv->_s4CA[bit]->get2D(v, i);
			assert(iob < (Int(1) << _bs));
			return io + iob;
		} else {
			return _buf->get(i);
		}
	}

	Int totalSize() {
		Int size = sizeof(*this);
		if (_isTree) {
			for (int i = 0; i < _h; ++i) {
				size += _buf[i].totalSize();
			}
			size += sizeof(decltype(_items)) * (_h - 1);
			size += sizeof(_SelectTreeTable::Item *) * _itemNum;
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

	// Build r1 & r2
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

	// Build r3
	Int row = Int(1) << _bs[2], column = _bs[2]; // r&c for r3 table
	for (Int i = 0; i < row; ++i) {
		for (Int j = 0; j < column; ++j) {
			_r3CA->set(i * column + j, getOnes(i >> (column - j - 1)));
		}
	}

	// Build index for select
	_n1 = rank1(_n - 1);
	_n0 = _n - _n1;

	_ssize1[0] = divceil(_n0, _bs[1]);
	_ssize1[1] = divceil(_n1, _bs[1]);

	_s1CA[0] = new WordRAM::CompressedArray(_ssize1[0], lgn);
	_s1CA[1] = new WordRAM::CompressedArray(_ssize1[1], lgn);
	_s2CA[0] = (_SelectBlockIndex *)malloc(_ssize1[0] * sizeof(_SelectBlockIndex));
	_s2CA[1] = (_SelectBlockIndex *)malloc(_ssize1[1] * sizeof(_SelectBlockIndex));

	// Initialize s3
	Int s3bs = lgn * lgn + 1;
	_s3CA = new _SelectTreeTable(s3bs);

	// Build s1 & s2
	Int cz = 0, co = 0; // currently we have `cz` zeros and `co` ones
	Int bz = 0, bo = 0; // currently we have `bz` blocks of zeros and `bo` blocks of ones
	Int lz = 0, lo = 0; // the beginging index of the current block

	_s1CA[0]->set(0, 0);
	_s1CA[1]->set(0, 0);
	for (Int i = 0; i < _n; ++i) {
		auto b = getBit(i);
		if (b == 0) {
			cz++;
		} else {
			co++;
		}

		if (cz == _bs[1]) {
			_s1CA[0]->set(bz + 1, i + 1);
			cz = 0;
			new (&_s2CA[0][bz]) _SelectBlockIndex(lz, i, this, 0, *_s3CA);
			bz++;
			lz = i + 1;
		}
		if (co == _bs[1]) {
			_s1CA[1]->set(bo + 1, i + 1);
			co = 0;
			new (&_s2CA[1][bo]) _SelectBlockIndex(lo, i, this, 1, *_s3CA);
			bo++;
			lo = i + 1;
		}
	}
	// build the last block
	if (lz != _n) {
		new (&_s2CA[0][bz]) _SelectBlockIndex(lz, _n - 1, this, 0, *_s3CA);
		bz++;
	}
	if (lo != _n) {
		new (&_s2CA[1][bo]) _SelectBlockIndex(lo, _n - 1, this, 1, *_s3CA);
		bo++;
	}
	assert(bz == _ssize1[0]);
	assert(bo == _ssize1[1]);

	_s3CA->finishInit();

	// Build s4
	Int s4bs = lgn / 2; // block size for s4 index
	Int s4r = Int(1) << s4bs, s4c = s4bs;
	Int s4wl = lgceil(s4bs) + 1; // word length of s4 index items, add 1 more validation bit
	_s4CA[0] = new WordRAM::CompressedArray2D(s4r, s4c, s4wl);
	_s4CA[1] = new WordRAM::CompressedArray2D(s4r, s4c, s4wl);
	assert(_s4CA[0]->maxValue() > s4bs);
	Int *s4buf = new Int[s4bs];
	for (int bit = 0; bit <= 1; ++bit) {
		for (int i = 0; i < s4r; ++i) {
			int bid = 0; // id in s4buf
			for (int k = 0; k < s4bs; ++k) {
				if (getBitIn(i, k, s4bs) == bool(bit)) { // s4 index is for 1
					s4buf[bid] = k;
					bid++;
				}
			}
			for (int j = 0; j < bid; ++j) {
				_s4CA[bit]->set(i * s4c + j, s4buf[j]);
			}
			for (int j = bid; j < s4bs; ++j) {
				_s4CA[bit]->set(i * s4c + j, _s4CA[bit]->maxValue());
			}
		}
	}
	delete[] s4buf;
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
	Int s1 = _s1CA[0]->get(i / _bs[1]);
	Int s2 = _s2CA[0][i / _bs[1]].select(i % _bs[1], 0);
	return s1 + s2;
}

Int BitVector::_select1BF(Int i) {
	return _s1BF[i];
}

Int BitVector::_select1CA(Int i) {
	Int s1 = _s1CA[1]->get(i / _bs[1]);
	Int s2 = _s2CA[1][i / _bs[1]].select(i % _bs[1], 1);
	return s1 + s2;
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
	free(_d);
	free(_r1BF); free(_s0BF); free(_s1BF);
	delete _r1CA; delete _r2CA; delete _r3CA;
	delete _s1CA[0]; delete _s1CA[1];
	for (int b = 0; b < 2; ++b) {
		for (int i = 0; i < _ssize1[b]; ++i) {
			_s2CA[b][i].destroy();
		}
	}
	free(_s2CA[0]); free(_s2CA[1]);
	delete _s3CA;
	delete _s4CA[0]; delete _s4CA[1];
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
			size += _s4CA[b]->totalSize();
		}
		size += _s3CA->totalSize();
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

	Int testLength = 139517;
	BitVector bv1(testLength, Type::BruteForce), bv2(testLength, Type::SuccinctWithCompressedArray);
	bv1.randomize(); bv2.randomize();
	bv1.buildIndex(); bv2.buildIndex();
	// Test rank
	for (int i = 0; i < testLength; ++i) {
		if (bv1.rank0(i) != bv2.rank0(i)) {
			std::cout << bv1.rank0(i);
			std::cout << " ";
			std::cout << bv2.rank0(i);
			std::cout << std::endl;
			assertError("Rank0 Test Failed!");
		}
		if (bv1.rank1(i) != bv2.rank1(i)) {
			std::cout << bv1.rank1(i);
			std::cout << " ";
			std::cout << bv2.rank1(i);
			std::cout << std::endl;
			assertError("Rank1 Test Failed!");
		}
	}
	
	// Test Select
	if (bv1._n0 != bv2._n0 || bv1._n1 != bv2._n1) {
		assertError("Max Select Numbers Test Failed!");
	}
	for (Int i = 1; i <= bv1._n0; ++i) {
		if (bv1.select0(i) != bv2.select0(i)) {
			std::cout << bv1._select0BF(i - 1);
			std::cout << " ";
			std::cout << bv2._select0CA(i - 1);
			std::cout << std::endl;
			assertError("Select0 Test Failed!");
		}
	}
	for (Int i = 1; i <= bv1._n1; ++i) {
		if (bv1.select1(i) != bv2.select1(i)) {
			std::cout << bv1._select1BF(i - 1);
			std::cout << " ";
			std::cout << bv2._select1CA(i - 1);
			std::cout << std::endl;
			assertError("Select1 Test Failed!");
		}
	}
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
