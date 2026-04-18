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

        bool Connect(const std::string &ip, unsigned short port);

        bool IsConnected();

        bool SendRequest(const Request *req_ptr);

        bool SendHeartBeat();

        Response *ReadResponse();

        void Close();

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