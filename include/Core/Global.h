#pragma once

#include <Core/Context.h>
#include <Config/Config.h>

class Global
{
public:
    /// <summary>
    /// Initializes global context.
    /// </summary>
    /// <param name="pContext"></param>
    static void Init(const Context::Ptr& pContext);
    static void Shutdown();
    
    static const Config& GetConfig();
    static Context::Ptr GetContext();
    static const Environment& GetEnvVars();
    static EEnvironmentType GetEnv();
};