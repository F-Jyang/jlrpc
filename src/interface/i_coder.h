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
        /// @brief 编码请求。将request转换为string用于发送
        /// @param request
        /// @return
        std::string EncodeRequest(const RequestPtr &request)
        {
            return impl_.EncodeRequest(request);
        }

        /// @brief 解码请求。成功返回Request，失败返回nullptr
        /// @param buffer
        /// @return
        RequestPtr DecodeRequest(asio::streambuf &buffer, std::size_t bytes_transfered) 
        {
            return impl_.DecodeRequest(buffer, bytes_transfered);
        }

        /// @brief 编码响应。将request转换为string用于发送
        /// @param response
        /// @return
        std::string EncodeResponse(const ResponsePtr &response) 
        {
            return impl_.EncodeResponse(response);
        }

        /// @brief 解码响应。成功返回Response，失败返回nullptr
        /// @param buffer
        /// @return
        ResponsePtr DecodeResponse(asio::streambuf &buffer, std::size_t bytes_transfered)
        {
            return impl_.DecodeResponse(buffer, bytes_transfered);
        }
    private:
        CodeImpl impl_;
    };
}