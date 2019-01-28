#include "Sender.h"

#include <Common/FunctionalUtil.h>
#include <uuid.h>

Sender::Sender(const INodeClient& nodeClient)
	: m_nodeClient(nodeClient)
{

}

/*
	1: Create Transaction **UUID** (for reference and maintaining correct state)
	2: Set **lock_height** for transaction kernel (current chain height)
	3: Select **inputs** using desired selection strategy
	4: Calculate sum **inputs** blinding factors **xI**
	5: Create **change_output**
	6: Select blinding factor **xC** for **change_output**
	7: Create lock function **sF** that locks **inputs** and stores **change_output** in wallet
	   and identifying wallet transaction log entry **TS** linking **inputs + outputs**
	   (Not executed at this point)
	8: Calculate **tx_weight**: MAX(-1 * **num_inputs** + 4 * (**num_change_outputs** + 1), 1)
		(+1 covers a single output on the receiver's side)
	9: Calculate **fee**:  **tx_weight** * 1_000_000 nG
	10: Calculate total blinding excess sum for all inputs and outputs **xS1** = **xC** - **xI** (private scalar)
	11: Select a random nonce **kS** (private scalar)
	12: Subtract random kernel offset **oS** from **xS1**. Calculate **xS** = **xS1** - **oS**
	13: Multiply **xS** and **kS** by generator G to create public curve points **xSG** and **kSG**
	14: Add values to **Slate** for passing to other participants: **UUID, inputs, change_outputs,**
		**fee, amount, lock_height, kSG, xSG, oS**
*/
std::unique_ptr<Slate> Sender::BuildSendSlate(Wallet& wallet, const uint64_t amount, const uint64_t fee, const std::string& message, const ESelectionStrategy& strategy) const
{
	// 1. Create Transaction UUID (for reference and maintaining correct state).
	const uuids::uuid slateId = uuids::uuid_system_generator()();

	// 2. Set lock_height for transaction kernel (current chain height).
	const uint64_t lockHeight = m_nodeClient.GetChainHeight() + 1;

	// 3. Select inputs using desired selection strategy.
	std::vector<WalletCoin> inputs = wallet.GetAvailableCoins(strategy, amount + fee);
	
	// 4. Calculate sum inputs blinding factors xI.
	auto getInputBlindingFactors = [](WalletCoin& input) -> BlindingFactor { return input.GetPrivateKey().GetPrivateKey(); };
	std::vector<BlindingFactor> inputBlindingFactors = FunctionalUtil::map<std::vector<BlindingFactor>>(inputs, getInputBlindingFactors);
	std::unique_ptr<BlindingFactor> pBlindingFactorSum = Crypto::AddBlindingFactors(inputBlindingFactors, std::vector<BlindingFactor>());

	// 5. Create change output.
	uint64_t inputTotal = 0;
	for (const WalletCoin& input : inputs)
	{
		inputTotal += input.GetAmount();
	}
	const uint64_t changeAmount = inputTotal - (amount + fee);

	// 6. Select blinding factor xC for change output.
	std::unique_ptr<WalletCoin> pChangeOutput = wallet.CreateBlindedOutput(changeAmount);

	// 7. Create lock function **sF** that locks **inputs** and stores **change_output** in wallet
	// and identifying wallet transaction log entry **TS** linking **inputs + outputs** (Not executed at this point)


	// 8. Calculate **tx_weight**: MAX(-1 * **num_inputs** + 4 * (**num_change_outputs** + 1), 1)
	// (+1 covers a single output on the receiver's side)

	// 9. Calculate **fee**:  **tx_weight** * 1_000_000 nG

	// 10.Calculate total blinding excess sum for all inputs and outputs **xS1** = **xC** - **xI** (private scalar)
	// TODO: Finish this.

	return std::unique_ptr<Slate>(nullptr);
}