#include "Wallet.h"
#include "Sender.h"

Wallet::Wallet(const Config& config, INodeClient& nodeClient)
	: m_config(config), m_nodeClient(nodeClient)
{

}

std::unique_ptr<Wallet> Wallet::Initialize(const Config& config, INodeClient& nodeClient, const SecureString& password)
{
	return std::make_unique<Wallet>(Wallet(config, nodeClient));
}

std::unique_ptr<Wallet> Wallet::Load(const Config& config, INodeClient& nodeClient, const SecureString& password)
{
	return std::make_unique<Wallet>(Wallet(config, nodeClient));
}

bool Wallet::Send(const uint64_t amount, const uint64_t fee, const std::string& message, const ESelectionStrategy& strategy, const ESendMethod& method, const std::string& destination)
{
	std::unique_ptr<Slate> pSlate = Sender(m_nodeClient).BuildSendSlate(*this, amount, fee, message, strategy);
	if (pSlate != nullptr)
	{
		// TODO: Send slate to destination using ESendMethod.
		return true;
	}

	return false;
}

std::vector<WalletCoin> Wallet::GetAvailableCoins(const ESelectionStrategy& strategy, const uint64_t amountWithFee)
{
	return std::vector<WalletCoin>();
}

std::unique_ptr<WalletCoin> Wallet::CreateBlindedOutput(const uint64_t amount)
{
	return std::unique_ptr<WalletCoin>(nullptr);
}