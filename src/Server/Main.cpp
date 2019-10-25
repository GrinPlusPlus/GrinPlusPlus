#include "Node/NodeDaemon.h"
#include "ShutdownManager.h"
#include "Wallet/WalletDaemon.h"
#include "civetweb/include/civetweb.h"

#include <Common/Util/ThreadUtil.h>
#include <Config/ConfigManager.h>
#include <Infrastructure/ThreadManager.h>
#include <Wallet/WalletManager.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <signal.h>
#include <stdio.h>
#include <thread>

static void SigIntHandler(int signum)
{
    printf("\n\nCtrl-C Pressed\n\n");
    ShutdownManager::GetInstance().Shutdown();
}

int main(int argc, char *argv[])
{
    ThreadManagerAPI::SetCurrentThreadName("MAIN");

    EEnvironmentType environment = EEnvironmentType::MAINNET;
    bool headless = false;
    for (int i = 0; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--floonet")
        {
            environment = EEnvironmentType::FLOONET;
        }
        if (std::string(argv[i]) == "--headless")
        {
            headless = true;
        }
    }

    if (!headless)
    {
        std::cout << "INITIALIZING...\n";
        std::cout << std::flush;
    }

    /* Initialize the civetweb library */
    mg_init_library(0);

    signal(SIGINT, SigIntHandler);
    signal(SIGTERM, SigIntHandler);
    signal(SIGABRT, SigIntHandler);
    signal(9, SigIntHandler);

    const Config config = ConfigManager::LoadConfig(environment);
    NodeDaemon node(config);
    INodeClient *pNodeClient = node.Initialize();

    WalletDaemon wallet(config, *pNodeClient);
    wallet.Initialize();

    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    while (true)
    {
        if (ShutdownManager::GetInstance().WasShutdownRequested())
        {
            break;
        }

        if (!headless)
        {
            const int secondsRunning =
                (int)(std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch() - startTime.time_since_epoch())
                          .count());
            node.UpdateDisplay(secondsRunning);
        }

        ThreadUtil::SleepFor(std::chrono::seconds(1), ShutdownManager::GetInstance().WasShutdownRequested());
    }

    if (!headless)
    {
#ifdef _WIN32
        std::system("cls");
#else
        std::system("clear");
#endif

        std::cout << "SHUTTING DOWN...";
    }

    wallet.Shutdown();
    node.Shutdown();

    /* Un-initialize the civetweb library */
    mg_exit_library();

    return 0;
}