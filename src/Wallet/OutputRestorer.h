#pragma once

#include <Wallet/NodeClient.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Core/Models/DTOs/OutputDTO.h>
#include <Crypto/BulletproofType.h>

// Forward Declarations
class KeyChain;
class RewoundProof;

class OutputRestorer
{
public:
	OutputRestorer(INodeClientConstPtr pNodeClient, const KeyChain& keyChain)
		: m_pNodeClient(pNodeClient), m_keyChain(keyChain) { }

	std::vector<OutputDataEntity> FindAndRewindOutputs(
		const std::shared_ptr<IWalletDB>& pBatch,
		const bool fromGenesis
	) const;

private:
	std::unique_ptr<OutputDataEntity> GetWalletOutput(
		const OutputDTO& output
	) const;

	OutputDataEntity BuildWalletOutput(
		const OutputDTO& output,
		const RewoundProof& rewoundProof,
		EBulletproofType type
	) const;

	INodeClientConstPtr m_pNodeClient;
	const KeyChain& m_keyChain;
};