#pragma once

#include <string>

// Forward Declarations
class Config;
class FullBlock;
class BlockHeader;
class Commitment;
class OutputIdentifier;
class IBlockChainServer;
class IBlockDB;
class Transaction;

class ITxHashSet
{
public:
	virtual bool IsUnspent(const OutputIdentifier& output) const = 0;
	virtual bool IsValid(const Transaction& transaction) const = 0;
	virtual bool Validate(const BlockHeader& header, const IBlockChainServer& blockChainServer, Commitment& outputSumOut, Commitment& kernelSumOut) = 0;
	virtual bool ApplyBlock(const FullBlock& block) = 0;
	virtual bool SaveOutputPositions() = 0;

	virtual bool Snapshot(const BlockHeader& header) = 0;
	virtual bool Rewind(const BlockHeader& header) = 0;
	virtual bool Commit() = 0;
	virtual bool Discard() = 0;
	virtual bool Compact() = 0;
};