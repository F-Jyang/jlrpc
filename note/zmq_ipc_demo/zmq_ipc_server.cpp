#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);

    // 使用ipc协议进行进程间通信
    socket.bind("ipc:///tmp/zmq_demo.ipc");

    std::cout << "Server started, listening on ipc:///tmp/zmq_demo.ipc..." << std::endl;

    while (true) {
        zmq::message_t request;
        auto result = socket.recv(request, zmq::recv_flags::none);

        if (result) {
            std::string req_str(static_cast<char*>(request.data()), request.size());
            std::cout << "Server received: " << req_str << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            std::string response = "pong: " + req_str;
            zmq::message_t reply(response.data(), response.size());
            socket.send(reply, zmq::send_flags::none);

            std::cout << "Server sent: " << response << std::endl;
        }
    }

    return 0;
}
