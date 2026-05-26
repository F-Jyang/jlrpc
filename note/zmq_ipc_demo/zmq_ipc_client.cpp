#include <zmq.hpp>
#include <iostream>
#include <string>
#include <chrono>

int main() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::req);

    // 使用ipc协议连接到服务器
    socket.connect("ipc:///tmp/zmq_demo.ipc");

    std::cout << "Client connected, sending requests..." << std::endl;

    for (int i = 0; i < 5; ++i) {
        std::string msg = "ping_" + std::to_string(i);
        zmq::message_t request(msg.data(), msg.size());
        socket.send(request, zmq::send_flags::none);

        std::cout << "Client sent: " << msg << std::endl;

        zmq::message_t reply;
        socket.recv(reply, zmq::recv_flags::none);

        std::string resp_str(static_cast<char*>(reply.data()), reply.size());
        std::cout << "Client received: " << resp_str << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Done." << std::endl;
    return 0;
}
