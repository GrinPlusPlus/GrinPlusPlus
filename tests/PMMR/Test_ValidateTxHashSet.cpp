#include <catch.hpp>

#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <BlockChain/BlockChainServer.h>
#include <TxPool/TransactionPool.h>

TEST_CASE("ValidateTxHashSet")
{
	ConfigPtr pConfig = Config::Default(EEnvironmentType::MAINNET);
	auto pDatabase = DatabaseAPI::OpenDatabase(*pConfig);
	TxHashSetManagerPtr pTxHashSetManager = std::make_shared<TxHashSetManager>(*pConfig);
	auto pTransactionPool = TxPoolAPI::CreateTransactionPool(*pConfig, pTxHashSetManager);
	IBlockChainServerPtr pBlockChain = BlockChainAPI::StartBlockChainServer(*pConfig, pDatabase->GetBlockDB(), pTxHashSetManager, pTransactionPool);
	auto pHeader = pBlockChain->GetBlockHeaderByHash(Hash::FromHex("00000495026666846c6c0ab1296d78dd033bd044eb790be3e77acaec13e7c8a7"));
	pTxHashSetManager->Close();

	ITxHashSetPtr pTxHashSet = pTxHashSetManager->LoadFromZip(*pConfig, "C:\\Users\\David\\AppData\\Local\\Temp\\rebuilt.txhashset_000004950266.zip", pHeader);
	SyncStatus status;
	pTxHashSet->ValidateTxHashSet(*pHeader, *pBlockChain, status);
}