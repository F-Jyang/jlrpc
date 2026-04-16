#pragma once

#include <net/timer_wheel.h>
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
        PbServer(asio::io_context& main_ioct, const std::string &ip, unsigned short port, std::size_t thread_size = std::thread::hardware_concurrency() * 2);

        void RegisterSerivce(const std::string& service_name, const PbServicePtr &service_ptr);

        void UnRegisterService(const std::string& service_name);

        void Start();

        void Stop();

    private:
        void OnSessionClose(const PbSessionPtr &session, const std::error_code& ec);
        void OnSessionRead(const PbSessionPtr &session, const Request* request, const std::error_code& ec);
        void OnSessionWrite(const PbSessionPtr &session, std::size_t bytes_transferred, const std::error_code& ec);

    private:
        std::unique_ptr<TimerWheel> timer_wheel_;
        std::unique_ptr<asio::steady_timer> timer_;
        std::unique_ptr<Server> tcp_server_;
        std::unique_ptr<PbRpcDispatcher> dispatcher_;
        std::mutex session_mutex_;
        std::unordered_map<int64_t, PbSessionPtr> session_map_;
    };
}