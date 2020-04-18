#pragma once

#include <TestServer.h>
#include <TestWallet.h>
#include <TorProcessManager.h>
#include <Wallet/WalletManager.h>
#include <API/Wallet/Owner/OwnerServer.h>

class TestWalletServer
{
public:
    using Ptr = std::shared_ptr<TestWalletServer>;

    TestWalletServer(const IWalletManagerPtr& pWalletManager, const OwnerServer::Ptr& pOwnerServer)
        : m_pWalletManager(pWalletManager), m_pOwnerServer(pOwnerServer), m_numUsers(0) { }

    static TestWalletServer::Ptr Create(const TestServer::Ptr& pTestServer)
    {
        auto pWalletManager = WalletAPI::CreateWalletManager(
            *pTestServer->GetConfig(),
            pTestServer->GetNodeClient()
        );
        auto pOwnerServer = OwnerServer::Create(TorProcessManager::GetProcess(0), pWalletManager);
        return std::make_shared<TestWalletServer>(pWalletManager, pOwnerServer);
    }

    const IWalletManagerPtr& GetWalletManager() const noexcept { return m_pWalletManager; }
    const OwnerServer::Ptr& GetOwnerServer() const noexcept { return m_pOwnerServer; }
    uint16_t GetOwnerPort() const noexcept { return 3421; }

    TestWallet::Ptr CreateUser(
        const std::string& username,
        const SecureString& password)
    {
        auto response = m_pWalletManager->InitializeNewWallet(
            CreateWalletCriteria(username, password, 24),
            TorProcessManager::GetProcess(m_numUsers++)
        );
        return TestWallet::Create(
            m_pWalletManager,
            response.GetToken(),
            response.GetListenerPort(),
            response.GetTorAddress()
        );
    }

    TestWallet::Ptr Login(
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

private:
    IWalletManagerPtr m_pWalletManager;
    OwnerServer::Ptr m_pOwnerServer;
    size_t m_numUsers;
};