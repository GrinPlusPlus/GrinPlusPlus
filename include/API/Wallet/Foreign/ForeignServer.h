#pragma once

#include <Common/Secure.h>
#include <Config/Config.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <Net/Tor/TorManager.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Keychain/KeyChain.h>
#include <API/Wallet/Foreign/Handlers/ReceiveTxHandler.h>
#include <API/Wallet/Foreign/Handlers/CheckVersionHandler.h>
#include <optional>

class ForeignServer
{
public:
    using Ptr = std::shared_ptr<ForeignServer>;

	ForeignServer(const Config& config, const RPCServerPtr& pRPCServer)
		: m_config(config), m_pRPCServer(pRPCServer), m_torAddress(std::nullopt) { }
	~ForeignServer()
	{
		if (m_torAddress.has_value())
		{
			TorManager::GetInstance(m_config).RemoveListener(m_torAddress.value());
		}
	}

    static ForeignServer::Ptr Create(const Config& config, IWalletManager& walletManager, const SessionToken& token)
    {
        RPCServerPtr pServer = RPCServer::Create(EServerType::PUBLIC, std::nullopt, "/v2/foreign");
        pServer->AddMethod("receive_tx", std::shared_ptr<RPCMethod>((RPCMethod*)new ReceiveTxHandler(walletManager, token)));
        pServer->AddMethod("check_version", std::shared_ptr<RPCMethod>((RPCMethod*)new CheckVersionHandler()));
        return std::shared_ptr<ForeignServer>(new ForeignServer(config, pServer));
    }

	std::optional<TorAddress> AddTorListener(const SecureVector& seed)
	{
		std::shared_ptr<TorAddress> pTorAddress = nullptr;
		try
		{
			KeyChain keyChain = KeyChain::FromSeed(m_config, seed);
			SecretKey64 torKey = keyChain.DeriveED25519Key(KeyChainPath::FromString("m/0/1/0"));

			pTorAddress =  TorManager::GetInstance(m_config).AddListener(torKey, GetPortNumber());
		}
		catch (std::exception& e)
		{
			WALLET_ERROR_F("Exception thrown: {}", e.what());
		}

		if (pTorAddress != nullptr)
		{
			m_torAddress = std::make_optional(*pTorAddress);
		}

		return m_torAddress;
	}

	uint16_t GetPortNumber() const noexcept { return m_pRPCServer->GetPortNumber(); }
	const std::optional<TorAddress>& GetTorAddress() const noexcept { return m_torAddress; }

private:
	const Config& m_config;
    RPCServerPtr m_pRPCServer;
	std::optional<TorAddress> m_torAddress;
};