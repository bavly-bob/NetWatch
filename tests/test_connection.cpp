#include "connection.hpp"
#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <string>
#include <vector>

using boost::asio::ip::tcp;
using namespace netwatch::networking;

namespace {
void failAndStop(boost::asio::io_context& io, const char* where, const boost::system::error_code& ec)
{
    ADD_FAILURE() << where << ": " << ec.message();
    io.stop();
}
} // namespace

TEST(ConnectionTest, SendReceive)
{
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::string received_message;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (ec) return failAndStop(io, "accept", ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        server_conn->setMessageHandler([&](const std::string& msg) {
            received_message = msg;
            io.stop();
        });
        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec) {
        if (ec) return failAndStop(io, "connect", ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();
        client_conn->send("hello world");
    });

    io.run();
    EXPECT_EQ(received_message, "hello world");
}

TEST(ConnectionTest, MultipleMessages)
{
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::vector<std::string> received_messages;
    constexpr int expected_count = 5;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (ec) return failAndStop(io, "accept", ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        server_conn->setMessageHandler([&](const std::string& msg) {
            received_messages.push_back(msg);
            if (static_cast<int>(received_messages.size()) == expected_count)
            {
                io.stop();
            }
        });
        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec) {
        if (ec) return failAndStop(io, "connect", ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();
        for (int i = 0; i < expected_count; ++i)
        {
            client_conn->send("msg_" + std::to_string(i));
        }
    });

    io.run();
    ASSERT_EQ(static_cast<int>(received_messages.size()), expected_count);
    for (int i = 0; i < expected_count; ++i)
    {
        EXPECT_EQ(received_messages[i], "msg_" + std::to_string(i));
    }
}

TEST(ConnectionTest, Echo)
{
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::string echo_response;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (ec) return failAndStop(io, "accept", ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        server_conn->setMessageHandler([&](const std::string& msg) { server_conn->send(msg); });
        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec) {
        if (ec) return failAndStop(io, "connect", ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->setMessageHandler([&](const std::string& msg) {
            echo_response = msg;
            io.stop();
        });
        client_conn->start();
        client_conn->send("echo_test");
    });

    io.run();
    EXPECT_EQ(echo_response, "echo_test");
}

TEST(ConnectionTest, LargeMessage)
{
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::string received_message;
    const std::string large_msg(1024 * 1024, 'A');

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (ec) return failAndStop(io, "accept", ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        server_conn->setMessageHandler([&](const std::string& msg) {
            received_message = msg;
            io.stop();
        });
        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec) {
        if (ec) return failAndStop(io, "connect", ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();
        client_conn->send(large_msg);
    });

    io.run();
    EXPECT_EQ(received_message.size(), large_msg.size());
    EXPECT_EQ(received_message, large_msg);
}

TEST(ConnectionTest, DisconnectHandlerFires)
{
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    bool disconnect_fired = false;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (ec) return failAndStop(io, "accept", ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        server_conn->setDisconnectHandler([&]() {
            disconnect_fired = true;
            io.stop();
        });
        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec) {
        if (ec) return failAndStop(io, "connect", ec);
        boost::system::error_code ignored;
        client_socket.shutdown(tcp::socket::shutdown_both, ignored);
        client_socket.close(ignored);
    });

    io.run();
    EXPECT_TRUE(disconnect_fired);
}

TEST(ConnectionTest, IsOpenState)
{
    boost::asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    const tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    bool was_open = false;
    bool was_closed_after_disconnect = true;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
        if (ec) return failAndStop(io, "accept", ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        was_open = server_conn->isOpen();
        server_conn->setDisconnectHandler([&]() {
            was_closed_after_disconnect = !server_conn->isOpen();
            io.stop();
        });
        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec) {
        if (ec) return failAndStop(io, "connect", ec);
        boost::system::error_code ignored;
        client_socket.shutdown(tcp::socket::shutdown_both, ignored);
        client_socket.close(ignored);
    });

    io.run();
    EXPECT_TRUE(was_open);
    EXPECT_TRUE(was_closed_after_disconnect);
}
