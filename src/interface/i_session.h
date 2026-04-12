/// @brief 会话类接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <net/net_data.h>
#include <asio/streambuf.hpp>
#include <asio/io_context.hpp>
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

        void SetTimeout(std::size_t timeout) {
            timeout_ = timeout;
        }

        std::size_t GetTimeout() const
        {
            return timeout_;
        }

		virtual void Close() = 0;

        virtual const asio::any_io_executor& GetIoExecutor() = 0;
        
        void SetContext(std::any context)
        {
            context_ = context;
        }

        std::any GetContext() const
        {
            return context_;
        }

    protected:
        std::size_t timeout_;
        std::any context_;
    };
}