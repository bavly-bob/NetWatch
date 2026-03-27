// handles one TCP connection - receives messages of format: <4-byte length><message>

#pragma once
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include "types.hpp"

namespace netwatch::networking {

class Connection : public std::enable_shared_from_this<Connection> {
public:
    explicit Connection(tcp::socket socket);
    void start();  // begin async read loop
    void send(const std::string& message);
    void setMessageHandler(MessageHandler handler); 
    void setDisconnectHandler(DisconnectHandler handler);
    bool isOpen() const; // check if connection is still open

private: // internal async handlers
    void readHeader();
    void readBody(std::size_t length);
    void handleError(const boost::system::error_code& ec);

private: // member variables
    tcp::socket socket_;
    std::array<char, 4> headerBuffer_;
    std::vector<char> bodyBuffer_;
    // callbacks
    MessageHandler messageHandler_;
    DisconnectHandler disconnectHandler_;
};

} // namespace netwatch::networking