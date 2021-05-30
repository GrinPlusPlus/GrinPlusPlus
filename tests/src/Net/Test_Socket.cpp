#include <catch.hpp>

#include <Net/Socket.h>
#include <Net/TestListener.h>

TEST_CASE("Socket")
{
    auto create_conn_fn = [](const asio_socket_ptr& pSocket) -> ITestConnection::Ptr {
        return std::make_shared<ITestConnection>(pSocket);
    };

    TestListener::UPtr pListener = TestListener::Create(create_conn_fn, 45678);
    pListener->RunInThread();

    asio::io_service io_service;
    asio_socket_ptr pSocket = std::make_shared<asio::ip::tcp::socket>(io_service);
    pSocket->connect(SocketAddress::Parse("127.0.0.1:45678").GetEndpoint());

    std::vector<uint8_t> bytes(50);
    pSocket->send(asio::buffer(bytes, bytes.size()));
    std::this_thread::sleep_for(std::chrono::seconds(1));
}