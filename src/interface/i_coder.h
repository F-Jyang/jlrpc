/// @brief 解析器接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <interface/i_data.h>
#include <asio/streambuf.hpp>
#include <optional>
#include <memory>

namespace jl
{
    class ICoder
    {
    public:
        /// @brief 编码请求。将request转换为string用于发送
        /// @param request
        /// @return
        virtual std::string EncodeRequest(const RequestPtr &request) = 0;

        /// @brief 解码请求。成功返回IRequest，失败返回std::nullopt_t
        /// @param buffer
        /// @return
        virtual std::optional<std::shared_ptr<IRequest>> DecodeRequest(asio::streambuf &buffer) = 0;

        /// @brief 编码响应。将request转换为string用于发送
        /// @param response
        /// @return
        virtual std::string EncodeResponse(const ResponsePtr &response) = 0;

        /// @brief 解码响应。成功返回IResponse，失败返回std::nullopt_t
        /// @param buffer
        /// @return
        virtual std::optional<std::shared_ptr<IResponse>> DecodeResponse(asio::streambuf &buffer) = 0;
    };
}