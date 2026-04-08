/// @brief 会话类接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <net/net_data.h>
#include <asio/streambuf.hpp>
#include <functional>
#include <chrono>

namespace jl
{
    class ISession;
    using SessionPtr = std::shared_ptr<ISession>;
    using SessionWeak = std::weak_ptr<ISession>;

    class ISession
    {
    public:
		virtual void Start() = 0;

		virtual std::size_t GetId() const = 0;

        virtual void WriteResponse(const ResponsePtr& response) = 0;

		virtual  void SetTimeout(std::size_t timeout) = 0;

		virtual std::size_t GetTimeout() const = 0;

		virtual void Close() = 0;

    };

    // class ISession;

    // using SessionPtr = std::shared_ptr<ISession>;

    // using RequestCallback = std::function<void(SessionPtr, const RequestPtr &)>;
    // using ResponseCallback = std::function<void(SessionPtr, const ResponsePtr &)>;

    // class ISession : public std::enable_shared_from_this<ISession>
    // {
    // public:
    //     virtual void Start() = 0;

    //     virtual std::size_t GetId() const = 0;

    //     virtual void SetRequestCallback(const RequestCallback &) = 0;

    //     virtual void SetResponseCallback(const ResponseCallback &) = 0;

    //     virtual std::size_t GetTimeout() const = 0;

    //     virtual void SetTimeout(std::size_t timeout) = 0;

    //     // /// @brief 关闭连接
    //     virtual void Close() = 0;

    //     // virtual void Read() = 0;

    //     // virtual void Write(const std::string &resp_str) = 0;

    //     // /// @brief 获取超时时间
    //     // virtual std::chrono::steady_clock::time_point GetTimeout() const = 0;

    //     // virtual void SetTimeout(const std::chrono::steady_clock::time_point&) = 0;

    //     // virtual SessionInfo GetSessionInfo() const = 0;

    //     // virtual void SetReadCallback(const ReadCallback &callback) = 0;

    //     // virtual void SetWriteCallback(const WriteCallback &callback) = 0;

    //     // virtual void SetCloseCallback(const CloseCallback &callback) = 0;
    // };
}