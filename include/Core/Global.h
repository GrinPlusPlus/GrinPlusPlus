#pragma once

#include <Core/Enums/Environment.h>
#include <Crypto/Hash.h>
#include <cstdint>
#include <atomic>
#include <memory>
#include <vector>

// Forward Declarations
class Context;
class Config;
class ICoinView;
class FullBlock;
class BlockHeader;
class TorProcess;

class Global
{
public:
    /// <summary>
    /// Initializes global context.
    /// </summary>
    /// <param name="pContext"></param>
    static void Init(const std::shared_ptr<Context>& pContext);
    static const std::atomic_bool& IsRunning();
    static void Shutdown();
    
    static Config& GetConfig();
    static std::shared_ptr<Context> GetContext();
    static std::shared_ptr<TorProcess> GetTorProcess();

    //
    // Environment
    //
    static Environment GetEnv();
    static bool IsMainnet() { return GetEnv() == Environment::MAINNET; }
    static bool IsTestnet() { return GetEnv() == Environment::FLOONET; }
    static bool IsAutomatedTesting() { return GetEnv() == Environment::AUTOMATED_TESTING; }
    static const std::vector<uint8_t>& GetMagicBytes();

    //
    // Genesis Block
    //
    static const FullBlock& GetGenesisBlock();
    static const std::shared_ptr<const BlockHeader>& GetGenesisHeader();
    static const Hash& GetGenesisHash();

    static void SetCoinView(const std::shared_ptr<const ICoinView>& pCoinView);
    static std::shared_ptr<const ICoinView> GetCoinView();

private:
    static std::shared_ptr<Context> LockContext();
};