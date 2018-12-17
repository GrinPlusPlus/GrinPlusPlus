#include "TxHashSetImpl.h"
#include "TxHashSetValidator.h"
#include "Zip/TxHashSetZip.h"

#include <HexUtil.h>
#include <FileUtil.h>
#include <StringUtil.h>
#include <BlockChainServer.h>
#include <Database/BlockDb.h>
#include <Infrastructure/Logger.h>

TxHashSet::TxHashSet(IBlockDB& blockDB, KernelMMR* pKernelMMR, OutputPMMR* pOutputPMMR, RangeProofPMMR* pRangeProofPMMR)
	: m_blockDB(blockDB), m_pKernelMMR(pKernelMMR), m_pOutputPMMR(pOutputPMMR), m_pRangeProofPMMR(pRangeProofPMMR)
{

}

TxHashSet::~TxHashSet()
{
	delete m_pOutputPMMR;
}

bool TxHashSet::IsUnspent(const OutputIdentifier& output) const
{
	const std::optional<uint64_t> mmrIndex = m_blockDB.GetOutputPosition(output.GetCommitment());
	if (mmrIndex.has_value())
	{
		std::unique_ptr<Hash> pMMRHash = m_pOutputPMMR->GetHashAt(mmrIndex.value());
		if (pMMRHash != nullptr)
		{
			Serializer serializer;
			output.Serialize(serializer);
			const Hash outputHash = Crypto::Blake2b(serializer.GetBytes());

			if (outputHash == *pMMRHash)
			{
				return true;
			}
		}
	}

	return false;
}

bool TxHashSet::Validate(const BlockHeader& header, const IBlockChainServer& blockChainServer, Commitment& outputSumOut, Commitment& kernelSumOut)
{
	LoggerAPI::LogInfo("TxHashSet::Validate - Validating TxHashSet for block " + HexUtil::ConvertHash(header.Hash()));
	const TxHashSetValidationResult result = TxHashSetValidator(blockChainServer).Validate(*this, header, outputSumOut, kernelSumOut);
	if (result.Successful())
	{
		LoggerAPI::LogInfo("TxHashSet::Validate - Successfully validated TxHashSet.");
		outputSumOut = result.GetOutputSum();
		kernelSumOut = result.GetKernelSum();
		return true;
	}

	return false;
}

bool TxHashSet::ApplyBlock(const FullBlock& block)
{
	return true;
}

bool TxHashSet::SaveOutputPositions()
{
	const uint64_t size = m_pOutputPMMR->GetSize();
	for (uint64_t mmrIndex = 0; mmrIndex < size; mmrIndex++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(mmrIndex);
		if (pOutput != nullptr)
		{
			m_blockDB.AddOutputPosition(pOutput->GetCommitment(), mmrIndex);
		}
	}

	return true;
}

bool TxHashSet::Snapshot(const BlockHeader& header)
{
	return true;
}

bool TxHashSet::Rewind(const BlockHeader& header)
{
	return true;
}

bool TxHashSet::Compact()
{
	return true;
}

namespace TxHashSetAPI
{
	TXHASHSET_API ITxHashSet* Open(const Config& config, IBlockDB& blockDB)
	{
		TxHashSet* pTxHashSet = nullptr; // TODO: Implement
		return pTxHashSet;
	}

	TXHASHSET_API ITxHashSet* LoadFromZip(const Config& config, IBlockDB& blockDB, const std::string& zipFilePath, const BlockHeader& header)
	{
		const TxHashSetZip zip(config);
		if (zip.Extract(zipFilePath, header))
		{
			LoggerAPI::LogInfo(StringUtil::Format("TxHashSetAPI::LoadFromZip - %s extracted successfully.", zipFilePath.c_str()));
			FileUtil::RemoveFile(zipFilePath);

			KernelMMR* pKernelMMR = KernelMMR::Load(config);
			pKernelMMR->Rewind(header.GetKernelMMRSize());
			pKernelMMR->Flush();

			OutputPMMR* pOutputPMMR = OutputPMMR::Load(config);
			pOutputPMMR->Rewind(header.GetOutputMMRSize());
			pOutputPMMR->Flush();

			RangeProofPMMR* pRangeProofPMMR = RangeProofPMMR::Load(config);
			pRangeProofPMMR->Rewind(header.GetOutputMMRSize());
			pRangeProofPMMR->Flush();

			return new TxHashSet(blockDB, pKernelMMR, pOutputPMMR, pRangeProofPMMR); // TODO: Just call Rewind(BlockHeader) on TxHashSet instead of each MMR
		}
		else
		{
			return nullptr;
		}
	}

	TXHASHSET_API void Close(ITxHashSet* pITxHashSet)
	{
		TxHashSet* pTxHashSet = (TxHashSet*)pITxHashSet;
		delete pITxHashSet;
	}
}