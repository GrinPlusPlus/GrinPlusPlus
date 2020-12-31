#pragma once

#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/Wallet.h>

// Forward Declarations
class Wallet;
class INodeClient;

class FinalizeSlateBuilder
{
public:
	FinalizeSlateBuilder(const std::shared_ptr<Wallet>& pWallet, const std::shared_ptr<const INodeClient>& pNode)
		: m_pWallet(pWallet), m_pNode(pNode) { }

	std::pair<Slate, Transaction> Finalize(
		const Slate& rcvSlate
	) const;

private:
	void AddPartialSignature(
		const SlateContextEntity& context,
		Slate& finalizeSlate,
		const Hash& kernelMessage
	) const;

	std::unique_ptr<Transaction> BuildTransaction(
		Slate& finalizeSlate,
		const Hash& kernelMessage,
		const Commitment& finalExcess
	) const;

	bool VerifyPaymentProofs(
		const WalletTx& walletTx,
		const Slate& finalizeSlate,
		const Commitment& finalExcess
	) const;

	void UpdateDatabase(
		const WalletTx& walletTx,
		const Slate& finalizeSlate
	) const;

	std::shared_ptr<Wallet> m_pWallet;
	std::shared_ptr<const INodeClient> m_pNode;
};