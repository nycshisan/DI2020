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
    BitVector bvBF(length, BitVector::Type::BruteForce), bvCA(length, BitVector::Type::SuccinctWithCompressedArray);
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
}

void TestBV() {
    std::cout << "Test Bit Vector" << std::endl;
    auto rg = RandomGenerator("Test Bit Vector");
    Int length = 1752193;
    TestBVImpl(rg, length);
}

void Playground() {
}