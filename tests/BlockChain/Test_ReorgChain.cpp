#include <catch.hpp>

#include "../TestServer.h"
#include "../TestMiner.h"
#include "../TxBuilder.h"
#include "../TestHelper.h"
#include "../TestModels.h"

#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>
#include <Core/Validation/TransactionValidator.h>
#include <Core/File/FileRemover.h>
#include <Consensus/Common.h>
#include <Core/Util/TransactionUtil.h>

TEST_CASE("Reorg Chain")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	TestMiner miner(pTestServer);
	KeyChain keyChain = KeyChain::FromRandom(*pTestServer->GetConfig());
	TxBuilder txBuilder(keyChain);
	auto pBlockChainServer = pTestServer->GetBlockChainServer();

	////////////////////////////////////////
	// Mine a chain with 30 blocks (Coinbase maturity for tests is only 25)
	////////////////////////////////////////
	std::vector<MinedBlock> minedChain = miner.MineChain(keyChain, 30);
	REQUIRE(minedChain.size() == 30);

	// Create a transaction that spends the coinbase from block 1
	TransactionOutput outputToSpend = minedChain[1].block.GetOutputs().front();
	TxBuilder::Input input({
		{ outputToSpend.GetFeatures(), outputToSpend.GetCommitment() },
		minedChain[1].coinbasePath.value(),
		minedChain[1].coinbaseAmount
	});
	TxBuilder::Output newOutput({
		KeyChainPath({ 1, 0 }),
		(uint64_t)10'000'000
	});
	TxBuilder::Output changeOutput({
		KeyChainPath({ 1, 1 }),
		(uint64_t)(minedChain[1].coinbaseAmount - 10'000'000)
	});

	Transaction spendTransaction = txBuilder.BuildTx(0, { input }, { newOutput, changeOutput });

	// Create block 30a
	Transaction coinbaseTx30a = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 30 }));

	// Combine transaction with coinbase transaction for block 29b
	TransactionPtr pCombinedTx30a = TransactionUtil::Aggregate({
		std::make_shared<Transaction>(coinbaseTx30a),
		std::make_shared<Transaction>(spendTransaction)
	});

	FullBlock block30a = miner.MineNextBlock(
		minedChain.back().block.GetBlockHeader(),
		*pCombinedTx30a
	);

	REQUIRE(pBlockChainServer->AddBlock(block30a) == EBlockChainStatus::SUCCESS);

	////////////////////////////////////////
	// Create "reorg" chain with block 28b that spends an earlier coinbase, and blocks 29b, 30b, and 31b on top
	////////////////////////////////////////
	Transaction coinbaseTx28b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 28 }));

	// Combine transaction with coinbase transaction for block 29b
	TransactionPtr pCombinedTx28b = TransactionUtil::Aggregate({
		std::make_shared<Transaction>(coinbaseTx28b),
		std::make_shared<Transaction>(spendTransaction)
	});

	// Create block 28b
	FullBlock block28b = miner.MineNextBlock(
		minedChain[27].block.GetBlockHeader(),
		*pCombinedTx28b
	);

	// Create block 29b
	Transaction coinbaseTx29b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 29 }));
	FullBlock block29b = miner.MineNextBlock(
		minedChain[27].block.GetBlockHeader(),
		coinbaseTx29b,
		{ block28b }
	);

	// Create block 30b
	Transaction coinbaseTx30b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 30 }));
	FullBlock block30b = miner.MineNextBlock(
		minedChain[27].block.GetBlockHeader(),
		coinbaseTx30b,
		{ block28b, block29b }
	);

	// Create block 31b
	Transaction coinbaseTx31b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 31 }));
	FullBlock block31b = miner.MineNextBlock(
		minedChain[27].block.GetBlockHeader(),
		coinbaseTx31b,
		{ block28b, block29b, block30b }
	);

	////////////////////////////////////////
	// Verify that block31a is added, but then replaced successfully with block31b & block32b
	////////////////////////////////////////
	REQUIRE(pBlockChainServer->AddBlock(block28b) == EBlockChainStatus::ORPHANED);
	REQUIRE(pBlockChainServer->AddBlock(block29b) == EBlockChainStatus::ORPHANED);
	REQUIRE(pBlockChainServer->AddBlock(block30b) == EBlockChainStatus::ORPHANED);
	REQUIRE(pBlockChainServer->AddBlock(block31b) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChainServer->GetBlockByHeight(31)->GetHash() == block31b.GetHash());

	// TODO: Assert unspent positions in leafset and in database.
}