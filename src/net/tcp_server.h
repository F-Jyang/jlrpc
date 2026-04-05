/// @brief 服务器类
/// @author Jyang.
/// @date 2026-1-17

// NOTE: pb_server、http_server 都将 tcp_server 作为一个成员变量，通过不同的回调函数从而实现不一样的行为

#pragma once

#include <net/io_context_pool.h>
#include <net/acceptor.h>
#include <thread>
#include <asio/signal_set.hpp>

namespace jl
{
  
    class Server : public std::enable_shared_from_this<Server>
    {
    public:
        Server(asio::io_context &main_ioct, const std::string &ip, unsigned short port, std::size_t ioct_pool_size = std::thread::hardware_concurrency() * 2);

        /// @brief 启动服务器
        /// @param thread_cnt 线程数量
        void Start();

        /// @brief 停止服务器
        void Stop();

        /// @brief 设置连接建立回调函数
        /// @param callback 连接建立回调函数
        void SetConnEstablishCallback(const ConnEstablishCallback &callback);

        ~Server();

    private:
        /// @brief 异步接受SIGINT停止信号
        void DoAwaitStop();

    private:
        bool is_stop_;
        asio::io_context &main_ioct_;
        // IoContextPool ioct_pool_;
        std::shared_ptr<Acceptor> acceptor_;
        asio::signal_set signals_;
    };
}