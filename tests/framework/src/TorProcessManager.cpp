#include <TorProcessManager.h>
#include <TestSingleton.h>

#include <Crypto/ED25519.h>
#include <Net/Connections/HttpConnection.h>
#include <Net/Tor/TorAddressParser.h>
#include <Net/Tor/TorConnection.h>

class MockTorConnection : public ITorConnection
{
public:
    MockTorConnection(HttpConnection::UPtr&& pConnection)
        : m_pConnection(std::move(pConnection)) { }

    RPC::Response Invoke(const RPC::Request& request, const std::string& location) final
    {
        return m_pConnection->Invoke(location, request);
    }

private:
    HttpConnection::UPtr m_pConnection;
};

class MockTorProcess : public ITorProcess
{
public:
    TorAddress AddListener(const ed25519_secret_key_t& secretKey, const uint16_t port_number) final
    {
        TorAddress address = TorAddressParser::FromPubKey(ED25519::CalculatePubKey(secretKey));
        m_services.insert({ address, port_number });
        return address;
    }

    bool RemoveListener(const TorAddress& address) final
    {
        auto iter = m_services.find(address);
        if (iter != m_services.end()) {
            m_services.erase(iter);
            return true;
        }

        return false;
    }

    std::shared_ptr<ITorConnection> Connect(const TorAddress& address) final
    {
        auto pConnection = HttpConnection::Connect("127.0.0.1", TorProcessManager::LookupPort(address));
        return std::make_shared<MockTorConnection>(std::move(pConnection));
    }

    bool RetryInit() final { return true; }

    bool GetPort(const TorAddress& address, uint16_t& port_number) const
    {
        auto iter = m_services.find(address);
        if (iter != m_services.end()) {
            port_number = iter->second;
            return true;
        }

        return false;
    }

private:
    std::map<TorAddress, uint16_t> m_services;
};

TorProcess::Ptr TorProcessManager::GetProcess(const size_t idx)
{
    TorProcessManager& manager = Get();

    while (manager.m_processes.size() < (idx + 1)) {
        //const uint64_t size = manager.m_processes.size();
        //const size_t socksPort = manager.m_processes.empty() ? 3422 : 20000 + (size * 2);
        //const size_t controlPort = manager.m_processes.empty() ? 3423 : 20000 + (size * 2) + 1;
        //auto pProc = TorProcess::Initialize(fs::temp_directory_path() / "grinpp_tor", (uint16_t)socksPort, (uint16_t)controlPort);
        //manager.m_processes.push_back(pProc);

        manager.m_processes.push_back(std::make_shared<MockTorProcess>());
    }

    return manager.m_processes[idx];
}

uint16_t TorProcessManager::LookupPort(const TorAddress& tor_address)
{
    uint16_t port_number = 0;

    for (const auto& pProc : Get().m_processes) {
        if (pProc->GetPort(tor_address, port_number)) {
            return port_number;
        }
    }

    return port_number;
}

TorProcessManager& TorProcessManager::Get()
{
    return test::SingleTon<TorProcessManager>::get();
}