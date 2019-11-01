#pragma once

#include <Crypto/Commitment.h>

namespace Traits
{
	class ICommitted
	{
	public:
		virtual ~ICommitted() = default;

		virtual const Commitment& GetCommitment() const = 0;
	};
}