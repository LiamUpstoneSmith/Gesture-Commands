#include <zmq.hpp>
#include <iostream>
#include <string>
#include <cstring>

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);
    socket.connect("tcp://localhost:5555");

    for (int i = 0; i < 10; i++) {
        sleep(1);
        std::string msg = "Hello";
        zmq::message_t request(msg.size());
        memcpy(request.data(), msg.data(), msg.size());
        socket.send(request, zmq::send_flags::none);

        zmq::message_t reply;
        auto result = socket.recv(reply, zmq::recv_flags::none);
        if (!result) {
            std::cerr << "recv failed" << std::endl;
            continue;
        }

        std::cout << "Received: "
                  << std::string(static_cast<char*>(reply.data()), reply.size())
                  << std::endl;
    }
    return 0;
}