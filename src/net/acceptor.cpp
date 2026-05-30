#include "acceptor.h"
#include <utils/easy_log.hpp>
#include <net/io_context_pool.h>

jl::Acceptor::Acceptor(asio::io_context &ioct, const std::string &ip, unsigned short port)
    : ioct_(ioct),
      acceptor_(ioct)
{
    LOG_DEBUG << "jlrpc listen on " << ip << ":" << std::to_string(port);
    net::tcp::endpoint endpoint(asio::ip::make_address(ip), port);
    try
    {
        acceptor_.open(endpoint.protocol());
        // allow address reuse
        acceptor_.set_option(asio::socket_base::reuse_address(true));
        // bind to server address
        acceptor_.bind(endpoint);
        // start listen for connection
        acceptor_.listen(asio::socket_base::max_listen_connections);
    }
    catch (const std::exception &error)
    {
        LOG_DEBUG << error.what();
        std::terminate();
    }
    catch (...)
    {
        LOG_DEBUG << "Unknown error.";
        std::terminate();
    }
    // LOG_WARN("Acceptor listen on {}:{}", endpoint.address().to_string(), endpoint.port());
}

void jl::Acceptor::OnAccept(asio::io_context &ioct, net::tcp::socket &&socket, const std::error_code &ec)
{
    if (ec)
    {
        assert(false);
        LOG_DEBUG << "OnAccept fail:" << ec.message();
        // return;
    }
    else
    {
        if (conn_establish_callback_)
        {
            AsyncConnPtr conn = std::make_shared<AsyncConnection>(ioct,std::move(socket),kDefaultBufferSize);
            conn_establish_callback_(conn);
        }
        else
        {
            std::error_code ignore_ec;
            socket.shutdown(asio::socket_base::shutdown_both, ignore_ec);
            socket.close(ignore_ec);
        }
    }
    DoAccept(); // 继续接受下一个连接
}

void jl::Acceptor::DoAccept()
{
    if (!acceptor_.is_open())
    {
        assert(false);
        return; // 直接返回
    }
    // 每个线程一个ioct，不需要使用strand来保证异步操作串行
    // auto socket_ptr = std::make_shared<net::tcp::socket>(asio::make_strand(ioct_));
    auto self(shared_from_this()); // 获取自身的shared_ptr，防止在异步操作中被销毁
    asio::io_context &ioct = IoContextPool::Instance().GetIoContext();
    acceptor_.async_accept(
        ioct,
        [&ioct, self](const std::error_code &ec, net::tcp::socket &&socket)
        {
            self->OnAccept(ioct, std::move(socket), ec);
        });
}

jl::Acceptor::~Acceptor()
{
    acceptor_.close();
}
