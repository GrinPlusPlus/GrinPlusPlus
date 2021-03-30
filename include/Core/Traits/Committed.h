#pragma once

#include <Crypto/Models/Commitment.h>

namespace Traits
{
	class ICommitted
	{
	public:
		virtual ~ICommitted() = default;

		virtual const Commitment& GetCommitment() const = 0;
	};
}