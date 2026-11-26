#pragma once
#include <random>

#define RAND(minValue, maxValue)		RandomRange(minValue, maxValue)

inline std::random_device rd;
inline std::mt19937 engine(rd());

inline int RandomRange(int minValue, int maxValue)
{
	std::uniform_int_distribution<> dist(minValue, maxValue);
	return dist(engine);
}
