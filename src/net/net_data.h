/// @brief 请求、响应及数据相关接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once

#include <interface/i_serialize.h>
#include <net/net_error_code.h>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace jl
{

    class Request;
    class Response;

    using RequestPtr = std::shared_ptr<Request>;
    using ResponsePtr = std::shared_ptr<Response>;

    constexpr int32_t kMaxRequestSize = 4 * 1024;
    constexpr int32_t kMaxResponseSize = 4 * 1024;

    struct Request
    {
    public:
        Request() noexcept;

        Request(std::string_view msg_id, std::string_view service_full_name, std::string_view param) noexcept;

        Request(std::string_view service_full_name, std::string_view param) noexcept;

        Request(Request &&other) noexcept;

        void SetMsgId(std::string_view msg_id);

        std::string_view GetMsgId() const;

        void SetServiceFullName(std::string_view service_full_name);

        std::string_view GetServiceFullName() const;

        void SetParam(const std::string_view &param);

        std::string_view GetParam() const;

        int32_t GetSize() const;

    private:
        // int32_t msg_id_len_;            // 消息id长度
        // int32_t service_full_name_len_; // 服务全名长度
        // int32_t pb_data_len_;           // pb数据长度
        std::string msg_id_;            // 消息id
        std::string service_full_name_; // 服务全名,service.function
        std::string param_;             // rpc参数
    };

    struct Response
    {
    public:
        Response() noexcept;

        Response(Response &&other) noexcept;

        Response(std::string_view msg_id, std::string_view pd_data, NetErrorCode error_code = NetErrorCode::kNoError) noexcept;

        void SetMsgId(std::string_view msg_id);

        std::string_view GetMsgId() const;

        void SetResult(std::string_view result);

        std::string_view GetResult() const;

        int32_t GetSize() const;

        void SetErrorCode(NetErrorCode ec);

        NetErrorCode GetErrorCode() const;

    private:
        // int32_t msg_id_len_;  // 消息id长度
        // int32_t pb_data_len_; // pb数据长度
        NetErrorCode error_code_; // 错误码
        std::string msg_id_;      // 消息id
        std::string result_;      // rpc结果
    };

    class HeartBeatRequest : public Request
    {
    public:
        explicit HeartBeatRequest() noexcept;
    };

    class HeartBeatResponse : public Response
    {
    public:
        explicit HeartBeatResponse() noexcept;
    };
}