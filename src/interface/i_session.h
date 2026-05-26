/// @brief 会话类接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <net/net_data.h>
#include <asio/streambuf.hpp>
#include <asio/io_context.hpp>
#include <functional>
#include <chrono>
#include <memory>
#include <any>

namespace jl
{
    class ISession;
    using SessionPtr = std::shared_ptr<ISession>;
    using SessionWeak = std::weak_ptr<ISession>;

    using RequestCallback = std::function<void(const SessionPtr &, const Request *, const std::error_code &ec)>;
    using ResponseCallback = std::function<void(const SessionPtr &, std::size_t bytes_transferred, const std::error_code &ec)>;
    using SessionCloseCallback = std::function<void(const SessionPtr &, const std::error_code &ec)>;

    class ISession : public std::enable_shared_from_this<ISession>
    {
    public:
        virtual void Start() = 0;

        virtual std::size_t GetId() const = 0;

        // /// @brief 获取指向自身的 shared_ptr<ISession>
        // /// @note 子类需实现，将 shared_from_this<子类> cast 为 SessionPtr
        // virtual SessionPtr GetSelfPtr() = 0;

        virtual void WriteResponse(const Response *response) = 0;

        /// @brief 设置超时时间并添加 TimerEvent
        /// @param timeout 超时秒数
        void SetTimeout(std::size_t timeout)
        {
            timeout_ = timeout;
        }

        std::size_t GetTimeout() const
        {
            return timeout_;
        }

        // /// @brief 刷新超时时间（重新添加 TimerEvent）
        // void RefreshTimeout();

        // /// @brief 取消超时事件
        // void CancelTimeout();

        virtual void Close() = 0;

        virtual const asio::any_io_executor &GetIoExecutor() = 0;

        void SetContext(std::any context)
        {
            context_ = context;
        }

        std::any GetContext() const
        {
            return context_;
        }

        /// @brief 调用request_callback_回调函数
        /// @param session_ptr
        /// @param req
        /// @param ec
        void OnRequestCallback(const SessionPtr &session_ptr, const Request *req, const std::error_code &ec)
        {
            if (request_callback_)
            {
                request_callback_(session_ptr, req, ec);
            }
        }

        /// @brief 调用reqponse_callback_回调函数
        /// @param session_ptr
        /// @param bytes_transferred
        /// @param ec
        void OnResponseCallback(const SessionPtr &session_ptr, std::size_t bytes_transferred, const std::error_code &ec)
        {
            if (response_callback_)
            {
                response_callback_(session_ptr, bytes_transferred, ec);
            }
        }

        /// @brief 调用session_close_callback_回调函数
        /// @param session_ptr
        /// @param ec
        void OnSessionCloseCallback(const SessionPtr &session_ptr, const std::error_code &ec)
        {
            if (close_callback_)
            {
                close_callback_(session_ptr, ec);
            }
        }

    protected:
        std::size_t timeout_;
        std::any context_;
        RequestCallback request_callback_;
        ResponseCallback response_callback_;
        SessionCloseCallback close_callback_;
    };
}