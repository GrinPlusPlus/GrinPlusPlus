#include "Server.h"

#include <string>
#include <iostream>
#include <filesystem>
#include <HeaderMMR.h>
#include <BlockChainServer.h>
#include <Database.h>
#include <Config/Config.h>
#include <TxHashSet.h>
#include <Config/ConfigManager.h>

void TestTxHashSetZip()
{
	Config config = ConfigManager::LoadConfig();
	IDatabase* pDatabase = DatabaseAPI::OpenDatabase(config);
	IBlockChainServer* pBlockChainServer = BlockChainAPI::StartBlockChainServer(config, *pDatabase);
	std::unique_ptr<BlockHeader> pHeader = pBlockChainServer->GetBlockHeaderByHeight(82172, EChainType::CANDIDATE); //57519

	//ITxHashSet* pTxHashSet = TxHashSetAPI::LoadFromZip(config, pDatabase->GetBlockDB(), config.GetDataDirectory() + "txhashset_snapshot_10427.zip", *pHeader);
	//if (pTxHashSet != nullptr)
	//{
	//	Commitment outputSum(CBigInteger<33>::ValueOf(0));
	//	Commitment kernelSum(CBigInteger<33>::ValueOf(0));
	//	pTxHashSet->Validate(*pHeader, *pBlockChainServer, outputSum, kernelSum);
	//}

	pBlockChainServer->ProcessTransactionHashSet(pHeader->Hash(), config.GetDataDirectory() + "txhashset_020bb57c.zip");

	BlockChainAPI::ShutdownBlockChainServer(pBlockChainServer);
	DatabaseAPI::CloseDatabase(pDatabase);
}

bool TestMMR()
{
	Config config = ConfigManager::LoadConfig();
	IDatabase* pDatabase = DatabaseAPI::OpenDatabase(config);
	IBlockChainServer* pBlockChainServer = BlockChainAPI::StartBlockChainServer(config, *pDatabase);
	IHeaderMMR* pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(config);

	const uint64_t totalHeight = pBlockChainServer->GetHeight(EChainType::CANDIDATE);
	for (uint64_t i = 0; i <= totalHeight; i++)
	{
		std::unique_ptr<BlockHeader> pHeader = pBlockChainServer->GetBlockHeaderByHeight(i, EChainType::CANDIDATE);
		//headerMMR.Rewind(i);
		//const Hash root = headerMMR.Root();
		//headerMMR.Rollback();
		//if (pHeader->GetPreviousRoot() != root)
		//{
		//	return false;
		//}

		pHeaderMMR->AddHeader(*pHeader);
	}

	pHeaderMMR->Commit();

	return true;
}

int main(int argc, char* argv[])
{
	std::cout << "INITIALIZING...\n";
	//TestMMR();
	//TestTxHashSetZip();

	Server server;
	server.Run();
	//server.Shutdown();

	return 0;
}