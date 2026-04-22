/// @brief PbClient
/// @author Jyang.
/// @date 2026-4-22

#pragma once

#include <net/tcp_connection.h>
#include <net/pb/pb_coder.h>
#include <utils/easy_log.hpp>
#include <asio/steady_timer.hpp>
#include <mutex>

namespace jl
{

    class PbClient;
    using PbClientPtr = std::shared_ptr<PbClient>;

    class PbClient : public std::enable_shared_from_this<PbClient>
    {
    public:
        PbClient(asio::io_context &ioct, std::size_t max_buffer_size = kDefaultBufferSize);

        /// @brief 连接到服务端
        /// @param ip 
        /// @param port 
        /// @return 
        bool Connect(const std::string &ip, unsigned short port);

        /// @brief 判断是否处于连接状态
        /// @return 
        bool IsConnected();

        /// @brief 发送request
        /// @param req_ptr 
        /// @return  
        bool SendRequest(const Request *req_ptr);

        /// @brief 发送心跳
        bool SendHeartBeat();

        /// @brief 读取回复
        Response *ReadResponse();

        /// @brief 关闭连接
        void Close();

        /// @brief 设置io操作的超时时间
        /// @param secs 
        void SetTimeout(int secs);

    private:

        void OnTimeout(const std::error_code &ec);

    private:
        asio::io_context &ioct_;
        ConnectionPtr connection_;
        asio::streambuf read_buffer_;
        int timeout_;
    };
}