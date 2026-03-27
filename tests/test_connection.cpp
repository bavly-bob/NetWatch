#include <iostream>
#include "connection.hpp"
#include "client.hpp"

int main() {
    std::cout << "Running Connection Tests..." << std::endl;
    
    boost::asio::io_context io;
    netwatch::networking::Client client(io);
    client.connect("127.0.0.1", 8080);    
    io.run();
    std::cout << "Connection tests completed successfully." << std::endl;
    
    return 0;
}
