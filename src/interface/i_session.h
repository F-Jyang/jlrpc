/// @brief 会话类接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <interface/i_data.h>
#include <asio/streambuf.hpp>
#include <functional>
#include <chrono>

namespace jl
{

    enum class ConnectionState
    {
        kActived = 0,
        kClosed,
    };

    class ISession;

    using SessionPtr = std::shared_ptr<ISession>;

    using ReadCallback = std::function<void(const SessionPtr&, asio::streambuf&)>;
    using WriteCallback = std::function<void(const SessionPtr &, std::size_t bytes_transferred)>;
    using CloseCallback = std::function<void(const SessionPtr &)>;

    class ISession
    {
    public:
        virtual int64_t GetId() const = 0;

        virtual void Read() = 0;

        virtual void Write(const std::string &resp_str) = 0;

        // /// @brief 获取超时时间
		// virtual std::chrono::steady_clock::time_point GetTimeout() const = 0;

		// virtual void SetTimeout(const std::chrono::steady_clock::time_point&) = 0;

        /// @brief 关闭连接
        virtual void Close() = 0;

        virtual void SetReadCallback(const ReadCallback &callback) = 0;

        virtual void SetWriteCallback(const WriteCallback &callback) = 0;

        virtual void SetCloseCallback(const CloseCallback &callback) = 0;
    };
}