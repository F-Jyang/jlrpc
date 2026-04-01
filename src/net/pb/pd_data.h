/// @brief protobuf协议数据包
/// @author Jyang.
/// @date 2026-3-30

#pragma once

#include <interface/i_data.h>
#include <stdint.h>
#include <optional>

namespace jl
{
    // total_len的字节数
    constexpr int kTotalLenSize = 4;

    struct PbRequest : public IRequest
    {
    public:
        void SetMsgId(const std::string &msg_id) override;
        std::string GetMsgId() const override;
        void SetServiceFullName(const std::string &service_full_name) override;
        std::string GetServiceFullName() const override;
        void SetParam(const std::string &param) override;
        std::string GetParam() const override;
        int32_t GetSize() const override;

        /// @brief 将被序列化为string的requst重写解序列化为request
        /// @param req_str
        /// @return
        bool ParseFromString(const std::string &req_str) override;
        std::string SerializeToString() override;

    private:
        // min_size is 16Bytes

        int32_t msg_id_len_;            // 消息id长度
        int32_t service_full_name_len_; // 服务全名长度
        int32_t pb_data_len_;           // pb数据长度
        std::string msg_id_;            // 消息id
        std::string service_full_name_; // 服务全名,service.function
        std::string pb_data_;           // pb数据
    };

    struct PbResponse : public IResponse
    {
    public:
        void SetMsgId(const std::string &msg_id) override;
        std::string GetMsgId() const override;
        void SetResult(const std::string &result) override;
        std::string GetResult() const override;
        int32_t GetSize() const override;
        void SetErrorCode(DataErrorCode ec);
        bool ParseFromString(const std::string &req_str) override;
        std::string SerializeToString() override;

    private:
        int32_t msg_id_len_;  // 消息id长度
        int32_t pb_data_len_; // pb数据长度
        int32_t error_code_;  // 错误码
        std::string msg_id_;  // 消息id
        std::string pb_data_; // pb数据
    };
}