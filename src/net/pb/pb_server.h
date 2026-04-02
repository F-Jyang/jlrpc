#pragma once

#include <net/tcp_server.h>
#include <net/pb/pb_coder.h>
#include <net/pb/pb_session.h>
#include <net/pb/pb_rpc_dispatcher.h>
#include <net/timer_wheel.h>
#include <asio/steady_timer.hpp>
#include <memory>
#include <mutex>

namespace jl
{
    class PbServer
    {
    public:
        PbServer(asio::io_context &ioct, const std::string &ip, unsigned short port);

        void Start();

        void Stop();

    private:
        void OnSessionClose(const SessionPtr &session_ptr);
        void OnSessionRead(const SessionPtr &ptr, asio::streambuf& buffer);
        void OnSessionWrite(const SessionPtr &ptr, std::size_t bytes_transferred);
    private:
        std::unique_ptr<asio::steady_timer> timer_;
        std::unique_ptr<Server> tcp_server_;
        std::unique_ptr<ICoder> pb_coder_;
        std::unique_ptr<PbRpcDispatcher> dispacher_;
        std::mutex session_mutex_;
        std::unordered_map<int64_t, SessionPtr> session_map_;
    };
}