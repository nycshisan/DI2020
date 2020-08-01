#include "PCH.h"

#include "Test.h"

#include "WordRAM.h"
#include "BitVector.h"

void SelfTest() {
    WordRAM::CompressedArray::Test();
    BitVector::Test();
}

void _TestCAImpl(RandomGenerator &rg, Int length, Int wordSize) {
    rg.reseed();
    Int *buf = new Int[length];
    Int maxValue = (Int(1) << wordSize) - 1;
    std::uniform_int_distribution<Int> vd(0, maxValue), rd(0, length - 1);
    for (int i = 0; i < length; ++i) {
        buf[i] = vd(rg);
    }
    Int *la = (Int *)malloc(length * sizeof(Int));
    WordRAM::CompressedArray ca(length, wordSize);

    // Test space
    std::cout << "Storing `" << length << "` length of `" << wordSize << "` bit data." << std::endl;
    std::cout << "Uncompressed size: " << length * sizeof(Int) << std::endl;
    std::cout << "Compressed size: " << ca.totalSize() << std::endl;

    // Test time
    MillisecondsTimer laTimer("Uncompressed Array Timer"), caTimer("Compressed Array Timer");
    std::cout << "Test reading:" << std::endl;
    for (int i = 0; i < length; ++i) {
        auto r = rd(rg);
        Int x;
        laTimer.start();
        x = la[r];
        laTimer.end();
        caTimer.start();
        x = ca.get(r);
        caTimer.end();
    }
    laTimer.print();
    caTimer.print();
    std::cout << "Test writing:" << std::endl;
    for (int i = 0; i < length; ++i) {
        auto v = vd(rg), r = rd(rg);
        laTimer.start();
        la[r] = v;
        laTimer.end();
        caTimer.start();
        ca.set(r, v);
        caTimer.end();
    }
    laTimer.print();
    caTimer.print();

    delete buf;
    free(la);
}

void TestCA() {
    std::cout << "Test Compressed Array" << std::endl;
    auto rg = RandomGenerator("Test Compressed Array");
    Int length = 1391729, wordSize;
    // Test short word size
    std::cout << "Test short word size:" << std::endl;
    wordSize = 7;
    _TestCAImpl(rg, length, wordSize);
    std::cout << std::endl;
    // Test long word size
    std::cout << "Test long word size:" << std::endl;
    wordSize = 47;
    _TestCAImpl(rg, length, wordSize);
    std::cout << std::endl;

    std::cout << std::endl;
}

void TestBVImpl(RandomGenerator &rg, Int length) {
    rg.reseed();
    BitVector bvBF(length, DataStructureType::BruteForce), bvCA(length, DataStructureType::SuccinctWithCompressedArray);
    bvBF.randomize(); bvCA.randomize();

    std::cout << "Test build index speed for `" << length << "` length:" << std::endl;
    MillisecondsTimer bfTimer("Build brute force index"), caTimer("Build succinct index");
    bfTimer.start();
    bvBF.buildIndex();
    bfTimer.end();
    bfTimer.print();
    caTimer.start();
    bvCA.buildIndex();
    caTimer.end();
    caTimer.print();

    // Test space
    std::cout << "Storing `" << length << "` bits data." << std::endl;
    std::cout << "Uncompressed size: " << bvBF.totalSize() << std::endl;
    std::cout << "Compressed size: " << bvCA.totalSize() << std::endl;

    // Test rank speed
    std::cout << "Test rank speed:" << std::endl;
    bfTimer.name = "Brute force ranking"; caTimer.name = "Succinct ranking";
    bfTimer.clear(); caTimer.clear();
    for (int i = 0; i < length; ++i) {
        bfTimer.start();
        Int r0bf = bvBF.rank0(i);
        Int r1bf = bvBF.rank1(i);
        bfTimer.end();
        caTimer.start();
        Int r0ca = bvCA.rank0(i);
        Int r1ca = bvCA.rank1(i);
        caTimer.end();
        assert(r0ca == r0bf && r1ca == r1bf);
    }
    bfTimer.print(); caTimer.print();

    // Test select speed
    assert(bvBF.n0 == bvCA.n0 && bvBF.n1 == bvCA.n1);
    std::cout << "Test select speed:" << std::endl;
    bfTimer.name = "Brute force selecting"; caTimer.name = "Succinct selecting";
    bfTimer.clear(); caTimer.clear();
    for (int i = 1; i <= bvBF.n0; ++i) {
        bfTimer.start();
        Int s0bf = bvBF.select0(i);
        bfTimer.end();
        caTimer.start();
        Int s0ca = bvCA.select0(i);
        caTimer.end();
        assert(s0ca == s0bf);
    }
    for (int i = 1; i <= bvBF.n1; ++i) {
        bfTimer.start();
        Int s1bf = bvBF.select1(i);
        bfTimer.end();
        caTimer.start();
        Int s1ca = bvCA.select1(i);
        caTimer.end();
        assert(s1ca == s1bf);
    }
    bfTimer.print(); caTimer.print();
}

void TestBV() {
    std::cout << "Test Bit Vector" << std::endl;
    auto rg = RandomGenerator("Test Bit Vector");
    Int length = 3013;
    TestBVImpl(rg, length);
}

void Playground() {
}