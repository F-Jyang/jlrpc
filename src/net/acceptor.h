/// @file acceptor.h
/// @brief 接受器类
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once

#include <net/tcp_connection.h>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>

namespace net = asio::ip;


namespace jl
{
    using ConnEstablishCallback = std::function<void(const ConnectionPtr&)>;

    class Acceptor : public std::enable_shared_from_this<Acceptor>
    {
    public:
        Acceptor(asio::io_context &ioct, const std::string &ip, unsigned short port);
 
        /// @brief 设置连接建立回调函数
        /// @param callback
        void SetConnEstablishCallback(ConnEstablishCallback callback) { conn_establish_callback_ = callback; }

        /// @brief 异步接收连接
        void DoAccept();

        ~Acceptor();

    private:
        /// @brief 处理接受连接的回调函数
        /// @param socket 连接套接字
        /// @param ec 错误码
        void OnAccept(asio::io_context& ioct, net::tcp::socket&& socket, const std::error_code &ec);

    private:
        asio::io_context &ioct_;
        net::tcp::acceptor acceptor_;
        ConnEstablishCallback conn_establish_callback_;
    };
}
