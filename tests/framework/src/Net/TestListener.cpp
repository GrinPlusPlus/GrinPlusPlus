#include <Net/TestListener.h>

TestListener::UPtr TestListener::Create(const CreateConnFunc& fn_create_connection, const uint16_t port_number)
{
    std::unique_ptr<TestListener> pListener(new TestListener());

    pListener->m_pAcceptor = std::make_shared<asio::ip::tcp::acceptor>(
        *pListener->m_pAsioContext,
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port_number)
    );
    pListener->Accept(fn_create_connection);

    return pListener;
}

TestListener::~TestListener()
{
    for (auto pConnection : m_connections) {
        pConnection->Disconnect();
    }

    m_connections.clear();

    asio::error_code ec;
    m_pSocket->shutdown(asio::socket_base::shutdown_both, ec);
    m_pSocket.reset();
    m_pAcceptor.reset();
    std::cout << "Stopping ASIO context..." << std::endl;
    m_pAsioContext->stop();
    m_pAsioContext->run(ec);
    std::cout << "ASIO Context stopped!" << std::endl;

    while (true) {
        std::unique_lock<std::mutex> lock(m_asioMutex);
        if (m_tasks == 0) {
            m_pAsioContext.reset();
            std::cout << "ASIO context destroyed!" << std::endl;
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void TestListener::Accept(const CreateConnFunc& fn_create_connection)
{
    std::unique_lock<std::mutex> lock(m_asioMutex);
    if (!m_pAsioContext) {
        return;
    }

    m_tasks++;

    m_pSocket = std::make_shared<asio::ip::tcp::socket>(*m_pAsioContext);
    m_pAcceptor->async_accept(
        *m_pSocket,
        std::bind(&TestListener::OnAccept, this, fn_create_connection, std::placeholders::_1)
    );
}

void TestListener::OnAccept(CreateConnFunc fn_create_connection, const asio::error_code& ec)
{
    if (ec) {
        std::cout << "OnAccept error: " << ec.message() << std::endl;
    } else {
        auto pConnection = fn_create_connection(m_pSocket);
        pConnection->OnInboundConnection();
        m_connections.push_back(pConnection);

        Accept(fn_create_connection);
    }

    m_tasks--;
}