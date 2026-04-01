#pragma once

#include <net/pb/pb_rpc_dispatcher.h>
#include <net/pb/pb_session.h>
#include <net/tcp_server.h>
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
        void OnSessionRead(const SessionPtr &ptr, const std::shared_ptr<IRequest> &req_ptr);
        void OnSessionWrite(const SessionPtr &ptr, std::size_t bytes_transferred);
    private:
        std::unique_ptr<Server> tcp_server_;
        std::unique_ptr<PbRpcDispatcher> dispacher_;
        std::mutex session_mutex_;
        std::unordered_map<int64_t, SessionPtr> session_map_;
    };
}