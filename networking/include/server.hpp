// listens for incoming connections, manages multiple clients, and broadcasts messages

#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <vector>
#include "connection.hpp"
#include "types.hpp"

namespace netwatch::networking {

class Server {
public:
    Server(io_context& io, uint16_t port);
    void start();
    std::vector<Ptr<Connection>> getConnections() const;

private: // internal async handlers
    void doAccept();

private: // member variables
    io_context& io_;
    tcp::acceptor acceptor_;
    std::vector<Ptr<Connection>> connections_;
};

} // namespace netwatch::networking