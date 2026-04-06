#include "connection.hpp"
#include <boost/asio.hpp>
#include <cassert>
#include <iostream>
#include <thread>

using boost::asio::ip::tcp;
using namespace netwatch::networking;

void test_send_receive()
{
    boost::asio::io_context io;

    tcp::acceptor acceptor(io, tcp::endpoint(boost::asio::ip::address_v4::loopback(), 0));
    tcp::endpoint endpoint = acceptor.local_endpoint();

    std::shared_ptr<Connection> server_conn;
    std::shared_ptr<Connection> client_conn;

    std::string received_message;

    // Accept connection (server side)
    acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket)
    {
        assert(!ec);
        server_conn = std::make_shared<Connection>(std::move(socket));

        server_conn->setMessageHandler([&](const std::string& msg) {
            received_message = msg;
            io.stop();
        });

        server_conn->start();
    });

    // Connect client
    tcp::socket client_socket(io);
    client_socket.async_connect(endpoint, [&](boost::system::error_code ec)
    {
        assert(!ec);
        client_conn = std::make_shared<Connection>(std::move(client_socket));
        client_conn->start();

        // Send test message
        client_conn->send("hello world");
    });

    io.run();

    assert(received_message == "hello world");
    std::cout << "Test send/receive passed\n";
}

int main() {
    test_send_receive();
    return 0;
}
