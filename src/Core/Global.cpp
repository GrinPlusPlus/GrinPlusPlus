#include <Core/Global.h>

#include <cassert>

static Context::Ptr GLOBAL_CONTEXT = nullptr;

void Global::Init(const Context::Ptr& pContext)
{
	GLOBAL_CONTEXT = pContext;
}

void Global::Shutdown()
{
	GLOBAL_CONTEXT.reset();
}

const Config& Global::GetConfig()
{
	assert(GLOBAL_CONTEXT != nullptr);

	return GLOBAL_CONTEXT->GetConfig();
}

Context::Ptr Global::GetContext()
{
	return GLOBAL_CONTEXT;
}

const Environment& Global::GetEnvVars()
{
	assert(GLOBAL_CONTEXT != nullptr);

	return GLOBAL_CONTEXT->GetConfig().GetEnvironment();
}

EEnvironmentType Global::GetEnv()
{
	assert(GLOBAL_CONTEXT != nullptr);

	return GLOBAL_CONTEXT->GetConfig().GetEnvironment().GetType();
}