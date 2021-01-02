#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/Hash.h>
#include <Core/Traits/Batchable.h>
#include <Core/Traits/Lockable.h>
#include <vector>

#define PMMR_API

// Forward Declarations
class Config;
class BlockHeader;

class IHeaderMMR : public Traits::IBatchable
{
public:
	virtual ~IHeaderMMR() = default;

	virtual void AddHeader(const BlockHeader& header) = 0;
	virtual Hash Root(const uint64_t nextHeight) const = 0;
	virtual void Rewind(const uint64_t nextHeight) = 0;
};

namespace HeaderMMRAPI
{
	PMMR_API std::shared_ptr<Locked<IHeaderMMR>> OpenHeaderMMR(const Config& config);
}