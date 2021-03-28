#include <TestServer.h>
#include <TorProcessManager.h>
#include <TestNodeClient.h>
#include <TestMiner.h>

#include <Core/Global.h>
#include <Database/Database.h>
#include <Net/Clients/RPC/RPCClient.h>
#include <Wallet/Keychain/KeyChain.h>

TestServer::TestServer(
	const std::shared_ptr<IDatabase>& pDatabase,
	const std::shared_ptr<Locked<TxHashSetManager>>& pTxHashSetManager,
	const ITransactionPool::Ptr& pTxPool,
	const std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
	const IBlockChain::Ptr& pBlockChain
) : m_pDatabase(pDatabase),
	m_pTxHashSetManager(pTxHashSetManager),
	m_pTxPool(pTxPool),
	m_pHeaderMMR(pHeaderMMR),
	m_pBlockChain(pBlockChain),
	m_pWalletManager(nullptr),
	m_pOwnerServer(nullptr),
	m_pMiner(nullptr),
	m_numUsers(0),
	m_tempDirs(0)
{

}

TestServer::~TestServer()
{
	m_pMiner.reset();
	m_pOwnerServer.reset();
	m_pWalletManager.reset();

	m_pBlockChain.reset();
	m_pHeaderMMR.reset();
	m_pTxPool.reset();
	m_pTxHashSetManager.reset();
	m_pDatabase.reset();

	auto dataPath = Global::GetConfig().GetDataDirectory();
	FileUtil::RemoveFile(dataPath / "NODE" / "CHAIN");
	FileUtil::RemoveFile(dataPath / "NODE" / "DB");
	FileUtil::RemoveFile(dataPath / "NODE" / "TXHASHSET");
	FileUtil::RemoveFile(dataPath / "NODE");
	FileUtil::RemoveFile(dataPath / "WALLET");
}

TestServer::Ptr TestServer::Create()
{
	const Config& config = Global::GetConfig();

	FileUtil::RemoveFile(config.GetChainPath());
	FileUtil::RemoveFile(config.GetDatabasePath());
	FileUtil::RemoveFile(config.GetTxHashSetPath());
	FileUtil::RemoveFile(config.GetWalletPath());

	FileUtil::CreateDirectories(config.GetChainPath());
	FileUtil::CreateDirectories(config.GetDatabasePath());
	FileUtil::CreateDirectories(config.GetTxHashSetPath());
	FileUtil::CreateDirectories(config.GetTxHashSetPath() / "kernel");
	FileUtil::CreateDirectories(config.GetTxHashSetPath() / "output");
	FileUtil::CreateDirectories(config.GetTxHashSetPath() / "rangeproof");
	FileUtil::CreateDirectories(config.GetWalletPath());

	IDatabasePtr pDatabase = DatabaseAPI::OpenDatabase(config);
	auto pTxHashSetManager = std::make_shared<Locked<TxHashSetManager>>(std::make_shared<TxHashSetManager>(config));
	ITransactionPool::Ptr pTxPool = TxPoolAPI::CreateTransactionPool(config);
	auto pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(config);
	IBlockChain::Ptr pBlockChain = BlockChainAPI::OpenBlockChain(
		config,
		pDatabase->GetBlockDB(),
		pTxHashSetManager,
		pTxPool,
		pHeaderMMR
	);

	return std::make_shared<TestServer>(
		pDatabase,
		pTxHashSetManager,
		pTxPool,
		pHeaderMMR,
		pBlockChain
	);
}

TestServer::Ptr TestServer::CreateWithWallet()
{
	auto pServer = Create();

	std::shared_ptr<INodeClient> pNodeClient(new TestNodeClient(
		pServer->m_pDatabase,
		*pServer->m_pTxHashSetManager,
		pServer->m_pTxPool,
		pServer->m_pBlockChain
	));

	pServer->m_pWalletManager = WalletAPI::CreateWalletManager(Global::GetConfig(), pNodeClient);
	pServer->m_pOwnerServer = OwnerServer::Create(TorProcessManager::GetProcess(0), pServer->m_pWalletManager);

	return pServer;
}

std::shared_ptr<Locked<IBlockDB>> TestServer::GetBlockDB() const noexcept { return m_pDatabase->GetBlockDB(); }

fs::path TestServer::GenerateTempDir()
{
	fs::path path = Global::GetConfig().GetDataDirectory() / "TEMP" / std::to_string(m_tempDirs++);
	fs::create_directories(path);
	return path;
}

RPC::Response TestServer::InvokeOwnerRPC(const std::string& method, const Json::Value& paramsJson)
{
	RPC::Request request = RPC::Request::BuildRequest(method, paramsJson);
	return HttpRpcClient().Invoke("127.0.0.1", "/v2", GetOwnerPort(), request);
}

CreatedUser TestServer::CreateUser(
	const std::string& username,
	const SecureString& password,
	const UseTor use_tor,
	const SeedWordSize num_words)
{
	auto response = m_pWalletManager->InitializeNewWallet(
		CreateWalletCriteria(StringUtil::ToLower(username), password, (uint8_t)num_words),
		use_tor == UseTor::YES ? TorProcessManager::GetProcess(m_numUsers++) : nullptr
	);
	auto pWallet = TestWallet::Create(
		m_pWalletManager,
		response.GetToken(),
		response.GetListenerPort(),
		response.GetTorAddress()
	);

	return CreatedUser{ pWallet, response.GetSeed() };
}

TestWallet::Ptr TestServer::Login(
	const TorProcess::Ptr& pTorProcess,
	const std::string& username,
	const SecureString& password)
{
	auto response = m_pWalletManager->Login(
		LoginCriteria(username, password),
		pTorProcess
	);
	return TestWallet::Create(
		m_pWalletManager,
		response.GetToken(),
		response.GetPort(),
		response.GetTorAddress()
	);
}

std::vector<MinedBlock> TestServer::MineChain(const uint64_t chain_height)
{
	if (m_pMiner == nullptr) {
		m_pMiner = std::make_unique<TestMiner>(shared_from_this());
	}

	return m_pMiner->MineChain(KeyChain::FromRandom(), chain_height);
}

std::vector<MinedBlock> TestServer::MineChain(const KeyChain& keychain, const uint64_t chain_height)
{
	if (m_pMiner == nullptr) {
		m_pMiner = std::make_unique<TestMiner>(shared_from_this());
	}

	return m_pMiner->MineChain(keychain, chain_height);
}

FullBlock TestServer::MineNextBlock(
	const BlockHeaderPtr& pPreviousHeader,
	const Transaction& transaction,
	const std::vector<FullBlock>& blocksToApply)
{
	if (m_pMiner == nullptr) {
		m_pMiner = std::make_unique<TestMiner>(shared_from_this());
	}

	return m_pMiner->MineNextBlock(pPreviousHeader, transaction, blocksToApply);
}