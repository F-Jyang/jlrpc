#pragma once

#include <interface/i_coder.h>
#include <net/net_data.h>
#include <asio/streambuf.hpp>

namespace jl
{

    /**
     * Pb数据格式如下：
	 *      Request: 
     *           ——————————————————————————————————————
     *         | std::string msg_id_  |	std::string service_full_name_ | std::string param_ |
     *           ——————————————————————————————————————
     *      PbCoder序列化后Request:
     *          ————————————————————————————————————————————
     *         | total_len | service_full_name_len | param_len | msg_id_ | service_full_name_ | param_  |
     *          ————————————————————————————————————————————
     * 
     *      Response:
     *          ———————————————————————————————————
	 *         | NetErrorCode error_code_ | std::string msg_id_ | std::string result_ |
     *          ———————————————————————————————————
     *      PbCoder序列化后的Response:
     *          ————————————————————————————————————
     *         | total_len | msg_id_len | result_len | msg_id_ | result_ | error_code_  |
     *          ————————————————————————————————————
     */

    class PbCoder : public Coder<PbCoder>
    {
    public:
        /// @brief 编码请求。将request转换为total_len+request_string用于发送
        /// @param request
        /// @return
        std::string EncodeRequest(const Request* request);

        /// @brief 解码请求。将request_string解码为Request，失败返回nullptr
        /// @param buffer
        /// @return
        Request* DecodeRequest(std::string_view req_str);

        /// @brief 编码响应。将response转换为total_len+response_string用于发送
        /// @param response
        /// @return
        std::string EncodeResponse(const Response* response);

        /// @brief 解码响应。将response_string解码为Response，失败返回nullptr
        /// @param buffer
        /// @return
        Response* DecodeResponse(std::string_view resp_str);
    };

    Coder<PbCoder>& GetPbCoder();
}