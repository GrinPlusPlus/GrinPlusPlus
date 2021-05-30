#pragma once

#include <Net/TestConnection.h>
#include <asio.hpp>
#include <thread>
#include <memory>
#include <mutex>

class TestListener
{
public:
    using UPtr = std::unique_ptr<TestListener>;
    using CreateConnFunc = std::function<ITestConnection::Ptr(const asio_socket_ptr&)>;

    static TestListener::UPtr Create(const CreateConnFunc& fn_create_connection, const uint16_t port_number);
    ~TestListener();

    void RunInThread() { std::thread([this]() { m_pAsioContext->run(); }).detach(); }

private:
    TestListener()
        : m_pAsioContext(std::make_shared<asio::io_service>()) { }

    void Accept(const CreateConnFunc& fn_create_connection);
    void OnAccept(CreateConnFunc fn_create_connection, const asio::error_code& ec);

    std::mutex m_asioMutex;
    std::atomic_uint32_t m_tasks;

    std::shared_ptr<asio::io_service> m_pAsioContext;
    std::shared_ptr<asio::ip::tcp::acceptor> m_pAcceptor;
    asio_socket_ptr m_pSocket;

    std::vector<ITestConnection::Ptr> m_connections;
};