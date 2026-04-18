/// @brief 解析器接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <net/net_data.h>
#include <asio/streambuf.hpp>
#include <optional>
#include <memory>

namespace jl
{
    template<typename CodeImpl>
    class Coder
    {
    public:
        /// @brief 编码请求。将request序列化为string用于发送，如果传入nullptr会导致崩溃
        /// @param request
        /// @return
        std::string EncodeRequest(const Request* request)
        {
            return impl_.EncodeRequest(request);
        }

        /// @brief 解码请求。成功返回Request，失败返回nullptr
        /// @param buffer
        /// @return
        Request* DecodeRequest(std::string_view req_str)
        {
            return impl_.DecodeRequest(req_str);
        }

        /// @brief 编码响应。将request转换为string用于发送，如果传入nullptr会导致崩溃
        /// @param response
        /// @return
        std::string EncodeResponse(const Response* response)
        {
            return impl_.EncodeResponse(response);
        }

        /// @brief 解码响应。成功返回Response，失败返回nullptr
        /// @param buffer
        /// @return
        Response* DecodeResponse(std::string_view resp_str)
        {
            return impl_.DecodeResponse(resp_str);
        }
    private:
        CodeImpl impl_;
    };
}