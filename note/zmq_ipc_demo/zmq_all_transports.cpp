#include <zmq.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <memory>

void run_inproc_demo() {
    std::cout << "\n=== inproc (线程间通信) ===" << std::endl;

    zmq::context_t context(1);
    zmq::socket_t sender(context, zmq::socket_type::push);
    zmq::socket_t receiver(context, zmq::socket_type::pull);

    sender.bind("inproc://demo_inproc");
    receiver.connect("inproc://demo_inproc");

    // 发送消息
    std::string msg = "hello from inproc";
    zmq::message_t zmq_msg(msg.data(), msg.size());
    sender.send(zmq_msg, zmq::send_flags::none);

    // 接收消息
    zmq::message_t reply;
    receiver.recv(reply, zmq::recv_flags::none);

    std::string resp(static_cast<char*>(reply.data()), reply.size());
    std::cout << "inproc received: " << resp << std::endl;
}

void run_ipc_demo() {
    std::cout << "\n=== ipc (进程间通信 - Unix Domain Socket) ===" << std::endl;

    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);

    socket.bind("ipc:///tmp/zmq_demo.ipc");
    std::cout << "Server listening on ipc:///tmp/zmq_demo.ipc" << std::endl;

    // 接收
    zmq::message_t request;
    socket.recv(request, zmq::recv_flags::none);
    std::string req(static_cast<char*>(request.data()), request.size());
    std::cout << "Server received: " << req << std::endl;

    // 回复
    std::string resp = "pong";
    zmq::message_t reply(resp.data(), resp.size());
    socket.send(reply, zmq::send_flags::none);
}

void run_tcp_demo() {
    std::cout << "\n=== tcp (网络通信) ===" << std::endl;

    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);

    socket.bind("tcp://127.0.0.1:5555");
    std::cout << "Server listening on tcp://127.0.0.1:5555" << std::endl;

    // 接收
    zmq::message_t request;
    socket.recv(request, zmq::recv_flags::none);
    std::string req(static_cast<char*>(request.data()), request.size());
    std::cout << "Server received: " << req << std::endl;

    // 回复
    std::string resp = "pong";
    zmq::message_t reply(resp.data(), resp.size());
    socket.send(reply, zmq::send_flags::none);
}

int main() {
    std::cout << "ZeroMQ 传输协议演示" << std::endl;

    // 1. inproc - 线程间通信
    run_inproc_demo();

    // 2. ipc - Unix Domain Socket
    std::thread(ipc_thread(run_ipc_demo)).detach();

    // 等待一下让ipc服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 客户端连接ipc
    {
        zmq::context_t context(1);
        zmq::socket_t client(context, zmq::socket_type::req);
        client.connect("ipc:///tmp/zmq_demo.ipc");

        std::string msg = "ping";
        zmq::message_t request(msg.data(), msg.size());
        client.send(request, zmq::send_flags::none);

        zmq::message_t reply;
        client.recv(reply, zmq::recv_flags::none);
        std::string resp(static_cast<char*>(reply.data()), reply.size());
        std::cout << "IPC Client received: " << resp << std::endl;
    }

    // 3. tcp - 网络通信
    std::thread(tcp_thread(run_tcp_demo)).detach();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    {
        zmq::context_t context(1);
        zmq::socket_t client(context, zmq::socket_type::req);
        client.connect("tcp://127.0.0.1:5555");

        std::string msg = "ping";
        zmq::message_t request(msg.data(), msg.size());
        client.send(request, zmq::send_flags::none);

        zmq::message_t reply;
        client.recv(reply, zmq::recv_flags::none);
        std::string resp(static_cast<char*>(reply.data()), reply.size());
        std::cout << "TCP Client received: " << resp << std::endl;
    }

    std::cout << "\n=== 所有传输方式演示完成 ===" << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
