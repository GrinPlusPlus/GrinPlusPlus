#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Traits/Printable.h>
#include <vector>

// See: https://github.com/mimblewimble/grin/blob/master/core/src/consensus.rs
namespace Consensus
{
	template <class T, typename = std::enable_if_t<std::is_base_of<Traits::IHashable, T>::value>>
	static bool IsSorted(const std::vector<T>& values)
	{
		for (auto iter = values.cbegin(); iter != values.cend(); iter++)
		{
			if (iter + 1 == values.cend())
			{
				break;
			}

			if (iter->GetHash() > (iter + 1)->GetHash())
			{
				return false;
			}
		}

		return true;
	}
}