#pragma once

#include <iostream>
#include <string>
#include <cassert>

[[noreturn]] inline void fatalError() {
	std::cerr << "Fatal Error!" << std::endl;
	exit(1);
}

#define assertError(msg) do {\
	std::cerr << msg << std::endl; \
	assert(false); \
} while(0)