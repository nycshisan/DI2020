#pragma once

#include <string>
#include <random>

typedef std::mt19937_64 RandomGeneratorType;

class RandomGenerator : public RandomGeneratorType {
	std::string _seed;

public:
	RandomGenerator(std::string seed_str) : _seed(seed_str) {
		reseed();
	}

	void reseed() {
		std::seed_seq ss(_seed.begin(), _seed.end());
		seed(ss);
	}
};