#include <Core/Global.h>
#include <Core/Context.h>
#include <Config/Config.h>

#include <csignal>
#include <cassert>

// A reference to SHARED_CONTEXT is kept until Shutdown() to make sure it doesn't get cleaned up.
static std::shared_ptr<Context> SHARED_CONTEXT;

static std::weak_ptr<Context> GLOBAL_CONTEXT;

static std::atomic_bool RUNNING = false;
static std::shared_ptr<const ICoinView> COIN_VIEW;

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

const Config& Global::GetConfig()
{
	return LockContext()->GetConfig();
}

Context::Ptr Global::GetContext()
{
	return GLOBAL_CONTEXT.lock();
}

const Environment& Global::GetEnvVars()
{
	return LockContext()->GetConfig().GetEnvironment();
}

EEnvironmentType Global::GetEnv()
{
	return LockContext()->GetConfig().GetEnvironment().GetType();
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
	assert(GLOBAL_CONTEXT != nullptr);

	auto pContext = GLOBAL_CONTEXT.lock();
	if (pContext == nullptr) {
		throw std::runtime_error("Failed to obtain global context.");
	}

	return pContext;
}