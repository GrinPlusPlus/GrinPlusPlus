#include <catch.hpp>

#include <TestServer.h>
#include <TestMiner.h>
#include <TestChain.h>
#include <TxBuilder.h>
#include <TestHelper.h>

#include <BlockChain/BlockChain.h>
#include <Database/Database.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>
#include <Core/Validation/TransactionValidator.h>
#include <Core/File/FileRemover.h>
#include <Consensus/Common.h>
#include <Core/Util/TransactionUtil.h>

//
// a - b - c
//  \
//   - b'
//
// Process in the following order -
// 1. block_a
// 2. header_b
// 3. header_b_fork
// 4. block_b_fork
// 5. block_b
// 6. block_c
//
TEST_CASE("REORG 1")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	KeyChain keyChain = KeyChain::FromRandom(*pTestServer->GetConfig());
	TxBuilder txBuilder(keyChain);
	auto pBlockChain = pTestServer->GetBlockChain();

	TestChain chain1(pTestServer);

	Test::Tx coinbase_a = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 1 }));
	MinedBlock block_a = chain1.AddNextBlock({ coinbase_a });

	Test::Tx coinbase_b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 2 }));
	MinedBlock block_b = chain1.AddNextBlock({ coinbase_b });

	Test::Tx coinbase_c = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 3 }));
	MinedBlock block_c = chain1.AddNextBlock({ coinbase_c });

	chain1.Rewind(2);
	Test::Tx coinbase_b_fork = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 2 }));
	MinedBlock block_b_fork = chain1.AddNextBlock({ coinbase_b_fork });

	////////////////////////////////////////
	// 1. block_a
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_a.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_a.block.GetHash());

	////////////////////////////////////////
	// 2. header_b
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlockHeader(block_b.block.GetHeader()) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b.block.GetHash());

	////////////////////////////////////////
	// 3. header_b_fork
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlockHeader(block_b_fork.block.GetHeader()) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b.block.GetHash());

	////////////////////////////////////////
	// 4. block_b_fork
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b_fork.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b.block.GetHash());

	////////////////////////////////////////
	// 5. block_b
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b.block.GetHash());

	////////////////////////////////////////
	// 6. block_c
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_c.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_c.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());
}

//
// a - b - c
//  \
//   - b'
//
// Process in the following order -
// 1. block_a
// 2. header_b
// 3. header_c
// 4. block_b_fork
// 5. block_c
// 6. block_b
//
TEST_CASE("REORG 2")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	KeyChain keyChain = KeyChain::FromRandom(*pTestServer->GetConfig());
	TxBuilder txBuilder(keyChain);
	auto pBlockChain = pTestServer->GetBlockChain();

	TestChain chain1(pTestServer);

	Test::Tx coinbase_a = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 1 }));
	MinedBlock block_a = chain1.AddNextBlock({ coinbase_a });

	Test::Tx coinbase_b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 2 }));
	MinedBlock block_b = chain1.AddNextBlock({ coinbase_b });

	Test::Tx coinbase_c = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 3 }));
	MinedBlock block_c = chain1.AddNextBlock({ coinbase_c });

	chain1.Rewind(2);
	Test::Tx coinbase_b_fork = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 2 }));
	MinedBlock block_b_fork = chain1.AddNextBlock({ coinbase_b_fork });

	////////////////////////////////////////
	// 1. block_a
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_a.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_a.block.GetHash());

	////////////////////////////////////////
	// 2. header_b
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlockHeader(block_b.block.GetHeader()) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b.block.GetHash());

	////////////////////////////////////////
	// 3. header_c
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlockHeader(block_c.block.GetHeader()) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());

	////////////////////////////////////////
	// 4. block_b_fork
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b_fork.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());

	////////////////////////////////////////
	// 5. block_c
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_c.block) == EBlockChainStatus::ORPHANED);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());

	////////////////////////////////////////
	// 6. block_b
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());

	////////////////////////////////////////
	// 7. Process orphans
	////////////////////////////////////////
	REQUIRE(pBlockChain->ProcessNextOrphanBlock());

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_c.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());
}

//
// a - b - c
//  \
//   - b'
//
// Process in the following order -
// 1. block_a
// 2. block_b
// 3. block_c
// 4. block_b_fork - higher difficulty
//
TEST_CASE("REORG 3")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	KeyChain keyChain = KeyChain::FromRandom(*pTestServer->GetConfig());
	TxBuilder txBuilder(keyChain);
	auto pBlockChain = pTestServer->GetBlockChain();

	TestChain chain1(pTestServer);

	Test::Tx coinbase_a = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 1 }));
	MinedBlock block_a = chain1.AddNextBlock({ coinbase_a });

	Test::Tx coinbase_b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 2 }));
	MinedBlock block_b = chain1.AddNextBlock({ coinbase_b });

	Test::Tx coinbase_c = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 3 }));
	MinedBlock block_c = chain1.AddNextBlock({ coinbase_c });

	chain1.Rewind(2);
	Test::Tx coinbase_b_fork = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 2 }));
	MinedBlock block_b_fork = chain1.AddNextBlock({ coinbase_b_fork }, 10);

	////////////////////////////////////////
	// Process block_a, block_b, block_c
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_a.block) == EBlockChainStatus::SUCCESS);
	REQUIRE(pBlockChain->AddBlock(block_b.block) == EBlockChainStatus::SUCCESS);
	REQUIRE(pBlockChain->AddBlock(block_c.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_c.block.GetHash());
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());

	////////////////////////////////////////
	// Process forked block_b with higher difficulty
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b_fork.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b_fork.block.GetHash());
}

//
// a - b - c
//  \
//   - b'
//
// Process in the following order -
// 1. block_a
// 2. block_b_fork
// 3. block_b
// 4. block_c
//
TEST_CASE("REORG 4")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	KeyChain keyChain = KeyChain::FromRandom(*pTestServer->GetConfig());
	TxBuilder txBuilder(keyChain);
	auto pBlockChain = pTestServer->GetBlockChain();

	TestChain chain1(pTestServer);

	Test::Tx coinbase_a = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 1 }));
	MinedBlock block_a = chain1.AddNextBlock({ coinbase_a });

	Test::Tx coinbase_b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 2 }));
	MinedBlock block_b = chain1.AddNextBlock({ coinbase_b });

	Test::Tx coinbase_c = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 3 }));
	MinedBlock block_c = chain1.AddNextBlock({ coinbase_c });

	chain1.Rewind(2);
	Test::Tx coinbase_b_fork = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 2 }));
	MinedBlock block_b_fork = chain1.AddNextBlock({ coinbase_b_fork });

	////////////////////////////////////////
	// 1. block_a
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_a.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_a.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 1);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_a.block.GetHash());

	////////////////////////////////////////
	// 2. block_b_fork
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b_fork.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b_fork.block.GetHash());

	////////////////////////////////////////
	// 3. block_b
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_b.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_b_fork.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 2);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_b_fork.block.GetHash());

	////////////////////////////////////////
	// 4. block_c
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block_c.block) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetHeight(EChainType::CONFIRMED) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED)->GetHash() == block_c.block.GetHash());
	REQUIRE(pBlockChain->GetHeight(EChainType::CANDIDATE) == 3);
	REQUIRE(pBlockChain->GetTipBlockHeader(EChainType::CANDIDATE)->GetHash() == block_c.block.GetHash());
}

TEST_CASE("Reorg Chain")
{
	TestServer::Ptr pTestServer = TestServer::Create();
	TestMiner miner(pTestServer);
	KeyChain keyChain = KeyChain::FromRandom(*pTestServer->GetConfig());
	TxBuilder txBuilder(keyChain);
	auto pBlockChain = pTestServer->GetBlockChain();

	////////////////////////////////////////
	// Mine a chain with 30 blocks (Coinbase maturity for tests is only 25)
	////////////////////////////////////////
	std::vector<MinedBlock> minedChain = miner.MineChain(keyChain, 30);
	REQUIRE(minedChain.size() == 30);

	// Create a transaction that spends the coinbase from block 1
	TransactionOutput outputToSpend = minedChain[1].block.GetOutputs().front();
	Test::Input input({
		{ outputToSpend.GetCommitment() },
		minedChain[1].coinbasePath.value(),
		minedChain[1].coinbaseAmount
	});
	Test::Output newOutput({
		KeyChainPath({ 1, 0 }),
		(uint64_t)10'000'000
	});
	Test::Output changeOutput({
		KeyChainPath({ 1, 1 }),
		(uint64_t)(minedChain[1].coinbaseAmount - 10'000'000)
	});

	Transaction spendTransaction = txBuilder.BuildTx(Fee(), { input }, { newOutput, changeOutput });

	// Create block 30a
	Test::Tx coinbaseTx30a = txBuilder.BuildCoinbaseTx(KeyChainPath({ 0, 30 }));

	// Combine transaction with coinbase transaction for block 29b
	TransactionPtr pCombinedTx30a = TransactionUtil::Aggregate({
		coinbaseTx30a.pTransaction,
		std::make_shared<Transaction>(spendTransaction)
	});

	FullBlock block30a = miner.MineNextBlock(
		minedChain.back().block.GetHeader(),
		*pCombinedTx30a
	);

	REQUIRE(pBlockChain->AddBlock(block30a) == EBlockChainStatus::SUCCESS);

	////////////////////////////////////////
	// Create "reorg" chain with block 28b that spends an earlier coinbase, and blocks 29b, 30b, and 31b on top
	////////////////////////////////////////
	Test::Tx coinbaseTx28b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 28 }));

	// Combine transaction with coinbase transaction for block 29b
	TransactionPtr pCombinedTx28b = TransactionUtil::Aggregate({
		coinbaseTx28b.pTransaction,
		std::make_shared<Transaction>(spendTransaction)
	});

	// Create block 28b
	FullBlock block28b = miner.MineNextBlock(
		minedChain[27].block.GetHeader(),
		*pCombinedTx28b
	);

	// Create block 29b
	Test::Tx coinbaseTx29b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 29 }));
	FullBlock block29b = miner.MineNextBlock(
		minedChain[27].block.GetHeader(),
		*coinbaseTx29b.pTransaction,
		{ block28b }
	);

	// Create block 30b
	Test::Tx coinbaseTx30b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 30 }));
	FullBlock block30b = miner.MineNextBlock(
		minedChain[27].block.GetHeader(),
		*coinbaseTx30b.pTransaction,
		{ block28b, block29b }
	);

	// Create block 31b
	Test::Tx coinbaseTx31b = txBuilder.BuildCoinbaseTx(KeyChainPath({ 1, 31 }));
	FullBlock block31b = miner.MineNextBlock(
		minedChain[27].block.GetHeader(),
		*coinbaseTx31b.pTransaction,
		{ block28b, block29b, block30b }
	);

	////////////////////////////////////////
	// Verify that block31a is added, but then replaced successfully with block31b & block32b
	////////////////////////////////////////
	REQUIRE(pBlockChain->AddBlock(block28b) == EBlockChainStatus::SUCCESS);
	REQUIRE(pBlockChain->AddBlock(block29b) == EBlockChainStatus::SUCCESS);
	REQUIRE(pBlockChain->AddBlock(block30b) == EBlockChainStatus::SUCCESS);
	REQUIRE(pBlockChain->AddBlock(block31b) == EBlockChainStatus::SUCCESS);

	REQUIRE(pBlockChain->GetBlockByHeight(31)->GetHash() == block31b.GetHash());

	// TODO: Assert unspent positions in leafset and in database.
}