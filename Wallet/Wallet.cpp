#include "Wallet.h"

#include "Keychain/KeyChain.h"

Wallet::Wallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username, KeyChainPath&& userPath)
	: m_config(config), m_nodeClient(nodeClient), m_walletDB(walletDB), m_keyChain(config), m_username(username), m_userPath(std::move(userPath))
{

}

Wallet* Wallet::LoadWallet(const Config& config, const INodeClient& nodeClient, IWalletDB& walletDB, const std::string& username)
{
	KeyChainPath userPath = KeyChainPath::FromString("m/0/0"); // TODO: Need to lookup actual account
	return new Wallet(config, nodeClient, walletDB, username, std::move(userPath));
}

std::vector<WalletCoin> Wallet::GetAllAvailableCoins(const CBigInteger<32>& masterSeed) const
{
	std::vector<WalletCoin> coins;

	std::vector<OutputData> outputs = m_walletDB.GetOutputs(m_username);
	for (OutputData& output : outputs)
	{
		if (output.GetStatus() == EOutputStatus::UNSPENT)
		{
			// CONFIG: Allow configurable number of confirmations.
			if (m_nodeClient.GetNumConfirmations(output.GetOutput().GetCommitment()) >= 10)
			{
				BlindingFactor blindingFactor = m_keyChain.DerivePrivateKey(masterSeed, output.GetKeyChainPath())->ToBlindingFactor();
				coins.emplace_back(WalletCoin(std::move(blindingFactor), std::move(output)));
			}
		}
	}

	return coins;
}

std::unique_ptr<WalletCoin> Wallet::CreateBlindedOutput(const CBigInteger<32>& masterSeed, const uint64_t amount)
{
	KeyChainPath keyChainPath = m_walletDB.GetNextChildPath(m_username, m_userPath);
	BlindingFactor blindingFactor = m_keyChain.DerivePrivateKey(masterSeed, keyChainPath)->ToBlindingFactor();
	std::unique_ptr<Commitment> pCommitment = Crypto::CommitBlinded(amount, blindingFactor);
	if (pCommitment != nullptr)
	{
		std::unique_ptr<RangeProof> pRangeProof = m_keyChain.GenerateRangeProof(masterSeed, keyChainPath, amount, *pCommitment, blindingFactor);
		if (pRangeProof != nullptr)
		{
			TransactionOutput transactionOutput(EOutputFeatures::DEFAULT_OUTPUT, Commitment(*pCommitment), RangeProof(*pRangeProof));
			OutputData outputData(std::move(keyChainPath), std::move(transactionOutput), amount, EOutputStatus::UNSPENT);

			return std::make_unique<WalletCoin>(WalletCoin(std::move(blindingFactor), std::move(outputData)));
		}
	}

	return std::unique_ptr<WalletCoin>(nullptr);
}

bool Wallet::SaveSlateContext(const uuids::uuid& slateId, const SlateContext& slateContext)
{
	return m_walletDB.SaveSlateContext(m_username, slateId, slateContext);
}

bool Wallet::LockCoins(std::vector<WalletCoin>& coins)
{
	std::vector<OutputData> outputs;

	for (WalletCoin& coin : coins)
	{
		coin.SetStatus(EOutputStatus::LOCKED);
		outputs.push_back(coin.GetOutputData());
	}

	return m_walletDB.AddOutputs(m_username, outputs);
}