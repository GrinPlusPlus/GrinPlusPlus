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
		auto it = vec.begin();
		std::advance(it, pos);
		vec.erase(it);
	}

	template<typename T>
	static bool Contains(const std::vector<T>& vec, const T& value)
	{
		for (T& t : vec)
		{
			if (t == value)
			{
				return true;
			}
		}

		return false;
	}

	template<typename T>
	static std::vector<T> Concat(const std::vector<T>& vec1, const std::vector<T>& vec2)
	{
		std::vector<T> result = vec1;
		result.reserve(vec1.size() + vec2.size());

		result.insert(result.end(), vec2.cbegin(), vec2.cend());

		return result;
	}
}