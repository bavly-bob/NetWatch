// initiates connection to server, sends messages, and disconnects

#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <string>
#include "connection.hpp"
#include "types.hpp"

namespace netwatch::networking {

class Client {
public:

    explicit Client(io_context& io);
    void connect(const std::string& host, uint16_t port);
    void send(const std::string& message);
    void disconnect();
    Ptr<Connection> getConnection() const; // get the current connection

private: // internal async handlers
    void doConnect(const tcp::resolver::results_type& endpoints);

private: // member variables
    io_context& io_;
    tcp::resolver resolver_;
    Ptr<Connection> connection_;
};

} // namespace netwatch::networking