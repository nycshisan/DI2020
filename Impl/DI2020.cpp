#include <iostream>

#include "WordRAM.h"
#include "BitVector.h"

void SelfTest() {
    WordRAM::CompressedArray::Test();
    BitVector::Test();
}

int main() {
    SelfTest();
}
