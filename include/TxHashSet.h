#pragma once

#include <ImportExport.h>
#include <string>

// Forward Declarations
class Config;
class FullBlock;
class BlockHeader;
class Commitment;
class OutputIdentifier;
class IBlockChainServer;
class IBlockDB;

#ifdef MW_PMMR
#define TXHASHSET_API __declspec(dllexport)
#else
#define TXHASHSET_API __declspec(dllimport)
#endif

class ITxHashSet
{
public:
	virtual bool IsUnspent(const OutputIdentifier& output) const = 0;
	virtual bool Validate(const BlockHeader& header, const IBlockChainServer& blockChainServer, Commitment& outputSumOut, Commitment& kernelSumOut) = 0;
	virtual bool ApplyBlock(const FullBlock& block) = 0;
	virtual bool SaveOutputPositions() = 0;

	virtual bool Snapshot(const BlockHeader& header) = 0;
	virtual bool Rewind(const BlockHeader& header) = 0;
	virtual bool Commit() = 0;
	virtual bool Discard() = 0;
	virtual bool Compact() = 0;
};

namespace TxHashSetAPI
{
	TXHASHSET_API ITxHashSet* Open(const Config& config, IBlockDB& blockDB);
	TXHASHSET_API ITxHashSet* LoadFromZip(const Config& config, IBlockDB& blockDB, const std::string& zipFilePath, const BlockHeader& header);
	TXHASHSET_API void Close(ITxHashSet* pTxHashSet);
}