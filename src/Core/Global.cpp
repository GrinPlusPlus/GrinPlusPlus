#include <Core/Global.h>
#include <Core/Context.h>
#include <Core/Genesis.h>
#include <Core/Config.h>

#include <csignal>
#include <cassert>

// A strong reference to SHARED_CONTEXT is kept until Shutdown() to make sure it doesn't get cleaned up.
static std::shared_ptr<Context> SHARED_CONTEXT;

// A weak reference is held to the context even after Shutdown(), so use this to access the context.
static std::weak_ptr<Context> GLOBAL_CONTEXT;

static std::atomic_bool RUNNING = false;
static std::shared_ptr<const ICoinView> COIN_VIEW;
static TorProcess::Ptr TOR_PROCESS;

static void SigIntHandler(int signum)
{
	printf("\n\n%d signal received\n\n", signum);
	Global::Shutdown();
}

void Global::Init(const Context::Ptr& pContext)
{
	SHARED_CONTEXT = pContext;
	GLOBAL_CONTEXT = pContext;
	RUNNING = true;

	const Config& config = pContext->GetConfig();

	if (pContext->GetEnvironment() != Environment::AUTOMATED_TESTING) {
		TOR_PROCESS = TorProcess::Initialize(
			config.GetTorDataPath(),
			config.GetSocksPort(),
			config.GetControlPort()
		);
	}

	signal(SIGINT, SigIntHandler);
	signal(SIGTERM, SigIntHandler);
	signal(SIGABRT, SigIntHandler);
	signal(9, SigIntHandler);
}

const std::atomic_bool& Global::IsRunning()
{
	return RUNNING;
}

void Global::Shutdown()
{
	RUNNING = false;
	
	SHARED_CONTEXT.reset();
}

Config& Global::GetConfig()
{
	return LockContext()->GetConfig();
}

Context::Ptr Global::GetContext()
{
	return GLOBAL_CONTEXT.lock();
}

TorProcess::Ptr Global::GetTorProcess()
{
	return TOR_PROCESS;
}

Environment Global::GetEnv()
{
	return LockContext()->GetEnvironment();
}

const std::vector<uint8_t>& Global::GetMagicBytes()
{
	return LockContext()->GetConfig().GetMagicBytes();
}

const FullBlock& Global::GetGenesisBlock()
{
	const Environment env = GetEnv();
	if (env == Environment::MAINNET) {
		return Genesis::MAINNET_GENESIS;
	} else {
		return Genesis::FLOONET_GENESIS;
	}
}

const std::shared_ptr<const BlockHeader>& Global::GetGenesisHeader()
{
	return GetGenesisBlock().GetHeader();
}

const Hash& Global::GetGenesisHash()
{
	return GetGenesisBlock().GetHash();
}

void Global::SetCoinView(const std::shared_ptr<const ICoinView>& pCoinView)
{
	COIN_VIEW = pCoinView;
}

std::shared_ptr<const ICoinView> Global::GetCoinView()
{
	return COIN_VIEW;
}

Context::Ptr Global::LockContext()
{
	auto pContext = GLOBAL_CONTEXT.lock();
	if (pContext == nullptr) {
		throw std::runtime_error("Failed to obtain global context.");
	}

	return pContext;
}