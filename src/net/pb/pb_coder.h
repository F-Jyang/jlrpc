#pragma once

#include <interface/i_coder.h>
#include <net/net_data.h>
#include <asio/streambuf.hpp>

namespace jl
{
    class PbCoder
    {
    public:
        /// @brief 编码请求。将request转换为total_len+request_string用于发送
        /// @param request
        /// @return
        std::string EncodeRequest(const Request* request);

        /// @brief 解码请求。将request_string解码为Request，失败返回nullptr
        /// @param buffer
        /// @return
        Request* DecodeRequest(const std::string& req_str);

        /// @brief 编码响应。将response转换为total_len+response_string用于发送
        /// @param response
        /// @return
        std::string EncodeResponse(const Response* response);

        /// @brief 解码响应。将response_string解码为Response，失败返回nullptr
        /// @param buffer
        /// @return
        Response* DecodeResponse(const std::string& resp_str);
    };

    Coder<PbCoder>& GetPbCoder();
}