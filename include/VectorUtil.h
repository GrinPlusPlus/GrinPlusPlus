#pragma once

#include <vector>
#include <string>

namespace VectorUtil
{
	template<typename T, size_t N>
	static std::vector<T> MakeVector(const T(&data)[N])
	{
		return std::vector<T>(data, data + N);
	}

	template <typename T>
	static void Remove(std::vector<T>& vec, const size_t pos)
	{
		std::vector<T>::iterator it = vec.begin();
		std::advance(it, pos);
		vec.erase(it);
	}
}