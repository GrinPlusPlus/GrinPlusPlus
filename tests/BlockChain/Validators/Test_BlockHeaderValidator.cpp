#include <catch.hpp>

#include <BlockChain/BlockChainServer.h>
#include <Config/ConfigManager.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

TEST_CASE("AggSig Interaction")
{
	Config config = ConfigManager::LoadConfig(EEnvironmentType::MAINNET);
	IDatabase* pDatabase = DatabaseAPI::OpenDatabase(config);
	TxHashSetManager txHashSetManager(config, pDatabase->GetBlockDB());
	ITransactionPool* pTxPool = TxPoolAPI::CreateTransactionPool(config, txHashSetManager, pDatabase->GetBlockDB());
	IBlockChainServer* pBlockChainServer = BlockChainAPI::StartBlockChainServer(config, *pDatabase, txHashSetManager, transactionPool);

}