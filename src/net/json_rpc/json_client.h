/// @brief JSON-RPC Client
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <net/tcp_connection.h>
#include <net/json_rpc/http_coder.h>
#include <net/json_rpc/json_coder.h>
#include <utils/easy_log.hpp>
#include <asio/steady_timer.hpp>
#include <mutex>

namespace jl
{

class JsonClient;
using JsonClientPtr = std::shared_ptr<JsonClient>;

class JsonClient : public std::enable_shared_from_this<JsonClient>
{
public:
    JsonClient(asio::io_context& ioct, std::size_t max_buffer_size = kDefaultBufferSize);

    bool Connect(const std::string& ip, unsigned short port);

    bool IsConnected();

    bool SendRequest(const Request* req_ptr);

    Response* ReadResponse();

    void Close();

    void SetTimeout(int secs);

private:
    void OnTimeout(const std::error_code& ec);

private:
    asio::io_context& ioct_;
    ConnectionPtr connection_;
    asio::streambuf read_buffer_;
    int timeout_{30};
};

} // namespace jl
