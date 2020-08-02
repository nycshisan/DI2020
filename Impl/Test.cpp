#include "PCH.h"

#include "Test.h"

#include "WordRAM.h"
#include "BitVector.h"

void SelfTest() {
    WordRAM::CompressedArray::Test();
    BitVector::Test();
}

void _TestCAImpl(RandomGenerator &rg, Int length, Int wordSize, std::ostream *logger = nullptr) {
    rg.reseed();
    Int *buf = new Int[length];
    Int maxValue = (Int(1) << wordSize) - 1;
    std::uniform_int_distribution<Int> vd(0, maxValue), rd(0, length - 1);
    for (int i = 0; i < length; ++i) {
        buf[i] = vd(rg);
    }
    Int *la = (Int *)malloc(length * sizeof(Int));
    WordRAM::CompressedArray ca(length, wordSize);

    if (logger) {
        *logger << length << " " << wordSize << " ";
    }

    // Test space
    if (logger) {
        *logger << length * sizeof(Int) << " " << ca.totalSize() << " ";
    } else {
        std::cout << "Storing `" << length << "` length of `" << wordSize << "` bit data." << std::endl;
        std::cout << "Uncompressed size: " << length * sizeof(Int) << std::endl;
        std::cout << "Compressed size: " << ca.totalSize() << std::endl;
    }

    // Test time
    NanosecondsTimer laTimer("Uncompressed Array Timer"), caTimer("Compressed Array Timer");
    if (logger == nullptr) std::cout << "Test reading:" << std::endl;
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
    if (logger) {
        *logger << laTimer.durationAvg(length) << " " << caTimer.durationAvg(length) << " ";
    } else {
        laTimer.printAvg(length);
        caTimer.printAvg(length);
    }
    laTimer.clear();
    caTimer.clear();
    if (logger == nullptr) std::cout << "Test writing:" << std::endl;
    for (int i = 0; i < length; ++i) {
        auto v = vd(rg), r = rd(rg);
        laTimer.start();
        la[r] = v;
        laTimer.end();
        caTimer.start();
        ca.set(r, v);
        caTimer.end();
    }
    if (logger) {
        *logger << laTimer.durationAvg(length) << " " << caTimer.durationAvg(length) << std::endl;
    } else {
        laTimer.printAvg(length);
        caTimer.printAvg(length);
    }

    delete[] buf;
    free(la);
}

void TestCA() {
    std::ofstream logger("../TestCAResult.txt");
    assert(logger.good());

    std::vector<Int> lengths = { 100, 200, 500 };
    Int lengthBase = 1;
    Int shortWordSize = 7, longWordSize = 47;

    std::cout << "Test Compressed Array:" << std::endl;
    auto rg = RandomGenerator("Test Compressed Array");
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < lengths.size(); ++j) {
            Int length = lengths[j] * lengthBase;
            std::cout << "Test size = " << length << std::endl;
            _TestCAImpl(rg, length, shortWordSize, &logger);
            _TestCAImpl(rg, length, longWordSize, &logger);
        }
        lengthBase *= 10;
    }

    std::cout << std::endl;
}

void _TestBVImpl(RandomGenerator &rg, Int length, std::ostream *logger = nullptr) {
    rg.reseed();
    BitVector bvBF(length, DataStructureType::BruteForce), bvCA(length, DataStructureType::SuccinctWithCompressedArray);
    bvBF.randomize(); bvCA.randomize();

    if (logger) {
        *logger << length << " ";
    } else {
        std::cout << "Test build index speed for `" << length << "` length:" << std::endl;
    }
    MicrosecondsTimer bfIndexTimer("Build brute force index"), caIndexTimer("Build succinct index");
    bfIndexTimer.start();
    bvBF.buildIndex();
    bfIndexTimer.end();
    if (logger) {
        *logger << bfIndexTimer.duration() << " ";
    } else {
        bfIndexTimer.print();
    }
    caIndexTimer.start();
    bvCA.buildIndex();
    caIndexTimer.end();
    if (logger) {
        *logger << caIndexTimer.duration() << " ";
    } else {
        caIndexTimer.print();
    }

    // Test space
    if (logger) {
        *logger << bvBF.totalSize() << " " << bvCA.totalSize() << " ";
    } else {
        std::cout << "Storing `" << length << "` bits data." << std::endl;
        std::cout << "Uncompressed size: " << bvBF.totalSize() << std::endl;
        std::cout << "Compressed size: " << bvCA.totalSize() << std::endl;
    }

    NanosecondsTimer bfTimer("Brute force ranking"), caTimer("Succinct ranking");
    // Test rank speed
    if (logger == nullptr) std::cout << "Test rank speed:" << std::endl;
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
    if (logger) {
        *logger << bfTimer.durationAvg(length) << " ";
        *logger << caTimer.durationAvg(length) << " ";
    } else {
        bfTimer.printAvg(length);
        caTimer.printAvg(length);
    }

    // Test select speed
    assert(bvBF.n0 == bvCA.n0 && bvBF.n1 == bvCA.n1);
    if (logger == nullptr) std::cout << "Test select speed:" << std::endl;
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
    if (logger) {
        *logger << bfTimer.durationAvg(bvBF.n0 + bvBF.n1) << " ";
        *logger << caTimer.durationAvg(bvCA.n0 + bvCA.n1) << std::endl;
    } else {
        bfTimer.printAvg(bvBF.n0 + bvBF.n1);
        caTimer.printAvg(bvCA.n0 + bvCA.n1);
    }
}

void TestBV() {
    std::ofstream logger("../TestBVResult.txt");
    assert(logger.good());

    std::vector<Int> lengths = { 1000, 2000, 5000 };
    Int lengthBase = 1;

    std::cout << "Test Bit Vector:" << std::endl;
    auto rg = RandomGenerator("Test Bit Vector");
    
    for (int i = 0; i < 5; ++i) {
        for (int j = 0; j < lengths.size(); ++j) {
            Int length = lengths[j] * lengthBase;
            std::cout << "Test size = " << length << std::endl;
            _TestBVImpl(rg, length, &logger);
        }
        lengthBase *= 10;
    }
    
    std::cout << std::endl;
}

void Playground() {
}