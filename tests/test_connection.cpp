// Connection tests - no external test framework, just assert + manual harness
// disclaimer: this test  was generate by antigravtiy and not yet tested

#include "connection.hpp"
#include <boost/asio.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

using boost::asio::ip::tcp;
using namespace netwatch::networking;

// ===================== HELPERS =====================

static int tests_passed = 0;
static int tests_failed = 0;

#define RUN_TEST(fn) do {                                           \
    std::cout << "  Running " << #fn << "... ";                     \
    try { fn(); ++tests_passed; std::cout << "PASSED\n"; }         \
    catch (const std::exception& e) {                               \
        ++tests_failed;                                             \
        std::cout << "FAILED: " << e.what() << "\n";               \
    }                                                               \
} while(0)

#define ASSERT_TRUE(cond) \
    if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) throw std::runtime_error(                       \
        std::string("Assertion failed: ") + #a + " != " + #b)

// ===================== TESTS =====================

// Test 1: Basic send and receive between two connections
void test_send_receive()
{
    boost::asio::io_context io;

    // Bind to port 0 → OS picks a free port
    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::string received_message;

    // Server side: accept one connection
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        ASSERT_TRUE(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));

        server_conn->setMessageHandler([&](const std::string& msg) {
            received_message = msg;
            io.stop();
        });

        server_conn->start();
    });

    // Client side: connect and send
    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        ASSERT_TRUE(!ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();
        client_conn->send("hello world");
    });

    io.run();

    ASSERT_EQ(received_message, std::string("hello world"));
}

// Test 2: Send multiple messages in sequence
void test_multiple_messages()
{
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::vector<std::string> received_messages;
    const int expected_count = 5;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        ASSERT_TRUE(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));

        server_conn->setMessageHandler([&](const std::string& msg) {
            received_messages.push_back(msg);
            if (static_cast<int>(received_messages.size()) == expected_count)
                io.stop();
        });

        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        ASSERT_TRUE(!ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();

        for (int i = 0; i < expected_count; ++i)
            client_conn->send("msg_" + std::to_string(i));
    });

    io.run();

    ASSERT_EQ(static_cast<int>(received_messages.size()), expected_count);
    for (int i = 0; i < expected_count; ++i)
        ASSERT_EQ(received_messages[i], "msg_" + std::to_string(i));
}

// Test 3: Bidirectional communication (echo)
void test_echo()
{
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::string echo_response;

    // Server echoes back whatever it receives
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        ASSERT_TRUE(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));

        server_conn->setMessageHandler([&](const std::string& msg) {
            server_conn->send(msg); // echo
        });

        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        ASSERT_TRUE(!ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));

        client_conn->setMessageHandler([&](const std::string& msg) {
            echo_response = msg;
            io.stop();
        });

        client_conn->start();
        client_conn->send("echo_test");
    });

    io.run();

    ASSERT_EQ(echo_response, std::string("echo_test"));
}

// Test 4: Large message transfer
void test_large_message()
{
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;
    std::string received_message;

    // 1 MB payload
    std::string large_msg(1024 * 1024, 'A');

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        ASSERT_TRUE(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));

        server_conn->setMessageHandler([&](const std::string& msg) {
            received_message = msg;
            io.stop();
        });

        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        ASSERT_TRUE(!ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();
        client_conn->send(large_msg);
    });

    io.run();

    ASSERT_EQ(received_message.size(), large_msg.size());
    ASSERT_EQ(received_message, large_msg);
}

// Test 5: Disconnect handler fires when peer closes
void test_disconnect_handler()
{
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    bool disconnect_fired = false;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        ASSERT_TRUE(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));

        server_conn->setDisconnectHandler([&]() {
            disconnect_fired = true;
            io.stop();
        });

        server_conn->start();
    });

    // Client connects then immediately closes
    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        ASSERT_TRUE(!ec);
        // Close immediately to trigger server disconnect
        boost::system::error_code ignored;
        client_socket.shutdown(tcp::socket::shutdown_both, ignored);
        client_socket.close(ignored);
    });

    io.run();

    ASSERT_TRUE(disconnect_fired);
}

// Test 6: isOpen() returns correct state
void test_is_open()
{
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    bool was_open = false;
    bool was_closed_after_disconnect = true;

    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        ASSERT_TRUE(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));
        was_open = server_conn->isOpen();

        server_conn->setDisconnectHandler([&]() {
            was_closed_after_disconnect = !server_conn->isOpen();
            io.stop();
        });

        server_conn->start();
    });

    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        ASSERT_TRUE(!ec);
        boost::system::error_code ignored;
        client_socket.shutdown(tcp::socket::shutdown_both, ignored);
        client_socket.close(ignored);
    });

    io.run();

    ASSERT_TRUE(was_open);
    ASSERT_TRUE(was_closed_after_disconnect);
}

// ===================== MAIN =====================

int main()
{
    std::cout << "=== Connection Tests ===\n";

    RUN_TEST(test_send_receive);
    RUN_TEST(test_multiple_messages);
    RUN_TEST(test_echo);
    RUN_TEST(test_large_message);
    RUN_TEST(test_disconnect_handler);
    RUN_TEST(test_is_open);

    std::cout << "\n--- Results: " 
              << tests_passed << " passed, " 
              << tests_failed << " failed ---\n";

    return tests_failed == 0 ? 0 : 1;
}
