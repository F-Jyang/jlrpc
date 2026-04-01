#pragma once

#include <interface/i_coder.h>

namespace jl
{
    class PbCoder : public ICoder
    {
    public:
        /// @brief 编码请求。将request转换为total_len+request_string用于发送
        /// @param request
        /// @return
        std::string EncodeRequest(const RequestPtr &request) override;

        /// @brief 解码请求。将request_string解码为IRequest，失败返回std::nullopt_t
        /// @param buffer
        /// @return
        std::optional<std::shared_ptr<IRequest>> DecodeRequest(asio::streambuf &buffer) override;

        /// @brief 编码响应。将response转换为total_len+response_string用于发送
        /// @param response
        /// @return
        std::string EncodeResponse(const ResponsePtr &response) override;

        /// @brief 解码响应。将response_string解码为IResponse，失败返回std::nullopt_t
        /// @param buffer
        /// @return
        std::optional<std::shared_ptr<IResponse>> DecodeResponse(asio::streambuf &buffer) override;
    };
}