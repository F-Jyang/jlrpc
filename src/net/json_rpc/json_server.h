/// @brief JSON-RPC Server
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <net/timer_wheel.h>
#include <net/tcp_server.h>
#include <net/json_rpc/json_session.h>
#include <net/json_rpc/json_rpc_dispatcher.h>
#include <memory>
#include <mutex>

namespace jl
{
    class JsonServer
    {
    public:
        JsonServer(asio::io_context& main_ioct, const std::string& ip, unsigned short port,
                   std::size_t thread_size = std::thread::hardware_concurrency() * 2);

        /// @brief 注册 RPC 方法
        /// @param method_name 方法名，格式 "service.method"
        /// @param handler 方法处理函数
        void RegisterMethod(const std::string& method_name, JsonRpcMethodHandler handler);

        void UnregisterMethod(const std::string& method_name);

        void Start();

        void Stop();

    private:
        void OnSessionClose(const JsonSessionPtr& session, const std::error_code& ec);
        void OnSessionRead(const JsonSessionPtr& session, const Request* request, const std::error_code& ec);
        void OnSessionWrite(const JsonSessionPtr& session, std::size_t bytes_transferred, const std::error_code& ec);

    private:
        std::unique_ptr<TimerWheel> timer_wheel_;
        std::unique_ptr<Server> tcp_server_;
        std::unique_ptr<JsonRpcDispatcher> dispatcher_;
        std::mutex session_mutex_;
        std::unordered_map<int64_t, JsonSessionPtr> session_map_;
    };
}
