#pragma once

#include <Crypto/BlindingFactor.h>
#include <Wallet/OutputData.h>

class WalletCoin
{
public:
	WalletCoin(BlindingFactor&& blindingFactor, OutputData&& outputData)
		: m_blindingFactor(std::move(blindingFactor)), m_outputData(std::move(outputData))
	{

	}

	inline const BlindingFactor& GetBlindingFactor() const { return m_blindingFactor; }
	inline const OutputData& GetOutputData() const { return m_outputData; }

	//
	// Operators
	//
	inline bool operator<(const WalletCoin& other) const { return m_outputData.GetAmount() < other.GetOutputData().GetAmount(); }

private:
	BlindingFactor m_blindingFactor;
	OutputData m_outputData;
};