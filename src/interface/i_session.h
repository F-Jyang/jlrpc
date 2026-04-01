/// @brief 会话类接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <interface/i_data.h>
#include <functional>
#include <memory>

namespace jl
{

    enum class ConnectionState
    {
        kActived = 0,
        kClosed,
    };

    class ISession;

    using SessionPtr = std::shared_ptr<ISession>;

    using ReadCallback = std::function<void(const SessionPtr &, const std::shared_ptr<IRequest> &)>;
    using WriteCallback = std::function<void(const SessionPtr &, std::size_t bytes_transferred)>;
    using CloseCallback = std::function<void(const SessionPtr &)>;

    class ISession
    {
    public:
        virtual int64_t GetId() const = 0;

        virtual void StartRead() = 0;

        virtual void Write(const ResponsePtr &resp_str) = 0;

        /// @brief 关闭连接
        virtual void Close() = 0;

        virtual void SetReadCallback(const ReadCallback &callback) = 0;

        virtual void SetWriteCallback(const WriteCallback &callback) = 0;

        virtual void SetCloseCallback(const CloseCallback &callback) = 0;
    };
}