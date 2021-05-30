#pragma once

#include <Net/SocketAddress.h>
#include <asio.hpp>
#include <iostream>
#include <memory>

using asio_socket_ptr = std::shared_ptr<asio::ip::tcp::socket>;

class ITestConnection : public std::enable_shared_from_this<ITestConnection>
{
public:
    using Ptr = std::shared_ptr<ITestConnection>;

    ITestConnection(const asio_socket_ptr& pSocket)
        : m_address(SocketAddress::From(*pSocket).Format()), m_pSocket(pSocket), m_tasks(0) { }
    virtual ~ITestConnection()
    {
        if (m_pSocket) {
            Disconnect();
        }
    }

    virtual void OnInboundConnection()
    {
        ReceiveAsync();
    }

    virtual void OnOutboundConnection()
    {
        ReceiveAsync();
    }

    virtual void Disconnect()
    {
        asio::error_code ec;
        std::cout << "Shutting down socket..." << std::endl;
        m_pSocket->shutdown(asio::socket_base::shutdown_both, ec);
        std::cout << "Closing socket..." << std::endl;
        m_pSocket->close(ec);
        std::cout << "Destroying socket..." << std::endl;

        while (true) {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (m_tasks == 0) {
                m_pSocket.reset();
                std::cout << "Socket Destroyed!" << std::endl;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
    virtual void ReceiveAsync()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_pSocket) {
            return;
        }

        m_received.resize(11);
        asio::async_read(
            *m_pSocket,
            asio::buffer(m_received, 11),
            std::bind(&ITestConnection::OnReceived, shared_from_this(), std::placeholders::_1, std::placeholders::_2)
        );

        m_tasks++;
    }

    virtual void OnReceived(const asio::error_code& ec, const size_t /*bytes_received*/)
    {        
        if (ec) {
            std::cout
                << "Error in OnReceived()!!!" << std::endl
                << "Socket: " << m_address << std::endl
                << "Error: " << ec.message() << std::endl << std::endl;
        } else {
            ProcessMessage(m_received);
            ReceiveAsync();
        }

        m_tasks--;
    }

    virtual void ProcessMessage(const std::vector<uint8_t>& message_bytes) const
    {
        std::cout
            << "Received " << message_bytes.size() << " bytes "
            << "from " << m_address << std::endl;
    }

private:
    std::mutex m_mutex;
    std::string m_address;
    asio_socket_ptr m_pSocket;
    std::vector<uint8_t> m_received;
    std::atomic_uint32_t m_tasks;
};