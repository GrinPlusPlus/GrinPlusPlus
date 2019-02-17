#pragma once

#include <algorithm>

class FunctionalUtil
{
public:
	template <typename Collection, typename unop>
	static void for_each(Collection col, unop op)
	{
		std::for_each(col.begin(), col.end(), op);
	}

	//template <typename Collection, typename unop>
	//static Collection map(Collection col, unop op)
	//{
	//	std::transform(col.begin(), col.end(), col.begin(), op);
	//	return col;
	//}
	template<typename RET_CONT, typename Collection, typename OPERATION>
	static RET_CONT map(Collection col, OPERATION op)
	{
		RET_CONT ret;
		std::transform(col.begin(), col.end(), std::back_inserter(ret), op);
		return ret;
	}

	template <typename Collection, typename Predicate>
	static Collection filterNot(Collection col, Predicate predicate)
	{
		auto returnIterator = std::remove_if(col.begin(), col.end(), predicate);
		col.erase(returnIterator, std::end(col));
		return col;
	}

	template <typename Collection, typename Predicate>
	static Collection filter(Collection col, Predicate predicate)
	{
		// capture the predicate in order to be used inside function
		auto fnCol = filterNot(col, [predicate](typename Collection::value_type i) { return !predicate(i); });
		return fnCol;
	}
};