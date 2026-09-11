#include <zmq.hpp>
#include <iostream>
#include <string>
#include <cstring>

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    socket.bind("tcp://*:5555");

    while (true) {
        zmq::message_t request;
        auto result = socket.recv(request, zmq::recv_flags::none);
        if (!result) {
            std::cerr << "recv failed" << std::endl;
            continue;
        }

        std::cout << "Received: "
                  << std::string(static_cast<char*>(request.data()), request.size())
                  << std::endl;

        std::string reply_str = "World";
        zmq::message_t reply(reply_str.size());
        memcpy(reply.data(), reply_str.data(), reply_str.size());
        socket.send(reply, zmq::send_flags::none);
    }
    return 0;
}