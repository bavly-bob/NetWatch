#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <functional>
#include <string>

namespace netwatch {

// Common aliases
using tcp = boost::asio::ip::tcp;
using io_context = boost::asio::io_context;
using MessageHandler = std::function<void(const std::string&)>; // callback for received messages
using DisconnectHandler = std::function<void()>; // callback for disconnects

// Smart pointers
template<typename T>
using Ptr = std::shared_ptr<T>;

}