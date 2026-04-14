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
    using ClientResponseCallabck = std::function<void(const PbClientPtr &, const Response*)>;
    using ClientRequestCallabck = std::function<void(const PbClientPtr &, std::size_t)>;
    using ClientCloseCallabck = std::function<void(const PbClientPtr &)>;

    enum class PbClientState : int32_t
    {
        kReadTotalLen = 0,
        kReadRequest,
    };

    class PbClient : public std::enable_shared_from_this<PbClient>
    {
    public:
        PbClient(asio::io_context &ioct, std::size_t max_buffer_size = kDefaultBufferSize);

        bool Connect(const std::string &ip, unsigned short port);

        void SendRequest(const Request* req_ptr);

        void SendHeartBeat();

        void ReadResponse();

        void Close();

        void SetReqeustCallback(const ClientRequestCallabck &callback)
        {
            write_callback_ = callback;
            std::weak_ptr<PbClient> weak = shared_from_this();
            connection_->SetWriteCallback(
                [weak](const ConnectionPtr &conn, std::size_t bytes_transferred)
                {
                    auto self = weak.lock();
                    if (!self)
                    {
                        return;
                    }
                    self->OnRequest(conn, bytes_transferred);
                });
        }

        void SetResponseCallback(const ClientResponseCallabck &callback)
        {
            read_callback_ = callback;
            std::weak_ptr<PbClient> weak = shared_from_this();
            connection_->SetReadCallback(
                [weak](const ConnectionPtr &conn, const std::string &resp_str)
                {
                    auto self = weak.lock();
                    if (!self)
                    {
                        return;
                    }
                    self->OnResponse(conn, resp_str);
                });
        }

        void SetCloseCallback(const ClientCloseCallabck &callback)
        {
            close_callback_ = callback;
            std::weak_ptr<PbClient> weak = shared_from_this();
            connection_->SetCloseCallback(
                [weak](const ConnectionPtr &conn)
                {
                    auto self = weak.lock();
                    if (!self)
                    {
                        return;
                    }
                });
        }

    private:
        void OnResponse(const ConnectionPtr &conn, const std::string &resp_str);

        void OnRequest(const ConnectionPtr &conn, std::size_t bytes_transferred);

    private:
        PbClientState state_;
        ConnectionPtr connection_;
        ClientRequestCallabck write_callback_;
        ClientResponseCallabck read_callback_;
        ClientCloseCallabck close_callback_;
        asio::streambuf read_buffer_;
        asio::steady_timer timer_;
    };
}