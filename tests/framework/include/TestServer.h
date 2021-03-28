#pragma once

#include <TestWallet.h>
#include <Models/MinedBlock.h>

#include <API/Wallet/Owner/OwnerServer.h>
#include <BlockChain/BlockChain.h>
#include <Core/File/FileRemover.h>
#include <Database/BlockDb.h>
#include <Net/Clients/RPC/RPC.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>
#include <Wallet/WalletManager.h>
#include <thread>

// Forward Declarations
class IDatabase;
class KeyChain;
class TestMiner;

enum class SeedWordSize : uint8_t
{
	WORDS_12 = 12,
	WORDS_15 = 15,
	WORDS_18 = 18,
	WORDS_21 = 21,
	WORDS_24 = 24
};

enum class UseTor
{
	YES,
	NO
};

struct CreatedUser
{
	TestWallet::Ptr wallet;
	SecureString seed_words;
};

class TestServer : public std::enable_shared_from_this<TestServer>
{
public:
	using Ptr = std::shared_ptr<TestServer>;

	TestServer(
		const std::shared_ptr<IDatabase>& pDatabase,
		const std::shared_ptr<Locked<TxHashSetManager>>& pTxHashSetManager,
		const ITransactionPool::Ptr& pTxPool,
		const std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
		const IBlockChain::Ptr& pBlockChain
	);
	~TestServer();

	static TestServer::Ptr Create();
	static TestServer::Ptr CreateWithWallet();

	std::shared_ptr<Locked<IBlockDB>> GetBlockDB() const noexcept;
	const std::shared_ptr<Locked<TxHashSetManager>>& GetTxHashSetManager() const noexcept { return m_pTxHashSetManager; }
	const ITransactionPool::Ptr& GetTxPool() const noexcept { return m_pTxPool; }
	const std::shared_ptr<Locked<IHeaderMMR>>& GetHeaderMMR() const noexcept { return m_pHeaderMMR; }
	const IBlockChain::Ptr& GetBlockChain() const noexcept { return m_pBlockChain; }

	fs::path GenerateTempDir();

	const IWalletManagerPtr& GetWalletManager() const noexcept { return m_pWalletManager; }
	const OwnerServer::UPtr& GetOwnerServer() const noexcept { return m_pOwnerServer; }
	uint16_t GetOwnerPort() const noexcept { return 3421; }

	RPC::Response InvokeOwnerRPC(const std::string& method, const Json::Value& paramsJson);

	CreatedUser CreateUser(
		const std::string& username,
		const SecureString& password,
		const UseTor use_tor = UseTor::YES,
		const SeedWordSize num_words = SeedWordSize::WORDS_24
	);

	TestWallet::Ptr Login(
		const TorProcess::Ptr& pTorProcess,
		const std::string& username,
		const SecureString& password
	);

	std::vector<MinedBlock> MineChain(const uint64_t chain_height);
	std::vector<MinedBlock> MineChain(const KeyChain& keychain, const uint64_t chain_height);

	FullBlock MineNextBlock(
		const BlockHeaderPtr& pPreviousHeader,
		const Transaction& transaction,
		const std::vector<FullBlock>& blocksToApply = {}
	);

private:
	// Server
	std::shared_ptr<IDatabase> m_pDatabase;
	std::shared_ptr<Locked<TxHashSetManager>> m_pTxHashSetManager;
	ITransactionPool::Ptr m_pTxPool;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	IBlockChain::Ptr m_pBlockChain;

	// Wallet server
	IWalletManagerPtr m_pWalletManager;
	OwnerServer::UPtr m_pOwnerServer;

	std::unique_ptr<TestMiner> m_pMiner;
	size_t m_tempDirs;
	size_t m_numUsers;
};