/// @brief 请求、响应及数据相关接口
/// @author Jyang.
/// @date 2026-4-2

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <interface/i_serialize.h>

namespace jl
{
    constexpr int32_t kMaxRequestSize = 4 * 1024;
    constexpr int32_t kMaxResponseSize = 4 * 1024;

    enum class DataErrorCode : int32_t
    {
        kNoError = 0,
        kUnRegisterFunction, // 函数未注册
        kInvaliadRequest,    // 请求解析异常
        kUnknown,            // 未知异常
    };

    const static std::unordered_map<DataErrorCode, std::string> kDataErrorMsgMap = {
        {DataErrorCode::kNoError, "No error."},
        {DataErrorCode::kUnRegisterFunction, "UnRegister function."},
        {DataErrorCode::kInvaliadRequest, "Invalid request."},
        {DataErrorCode::kUnknown, "Unknown error type."},
    };

    inline std::string GetPbErrorMsg(DataErrorCode error_type)
    {
        if (kDataErrorMsgMap.find(error_type) != kDataErrorMsgMap.end())
        {
            return kDataErrorMsgMap.find(error_type)->second;
        }
        return "Unknown error type.";
    }

    struct IRequest: public ISerialize
    {
        virtual void SetMsgId(const std::string &msg_id) = 0;

        /// @brief 获取消息id
        /// @return
        virtual std::string GetMsgId() const = 0;

        virtual void SetServiceFullName(const std::string &service_name) = 0;

        /// @brief 获取服务名称，service.function
        /// @return
        virtual std::string GetServiceFullName() const = 0;

        /// @brief 设置参数
        /// @param param
        virtual void SetParam(const std::string &param) = 0;

        /// @brief 获取rpc请求的参数
        /// @return
        virtual std::string GetParam() const = 0;

        /// @brief 获取request的size
        /// @return
        virtual int32_t GetSize() const = 0;

        // /// @brief 将string解序列化为request
        // /// @param req_str
        // /// @return
        // virtual bool ParseFromString(const std::string &req_str) = 0;

        // /// @brief 将request序列化为string
        // /// @return
        // virtual std::string SerializeToString() = 0;
    };

    struct IResponse : public ISerialize
    {
        /// @brief 设置msg_id
        /// @param msg_id
        virtual void SetMsgId(const std::string &msg_id) = 0;

        /// @brief 获取消息id
        /// @return
        virtual std::string GetMsgId() const = 0;

        /// @brief 设置调用结果
        /// @param result
        virtual void SetResult(const std::string &result) = 0;

        /// @brief 获取序列化为string的函数调用结果
        /// @return
        virtual std::string GetResult() const = 0;

        /// @brief 设置调用错误码
        /// @param ec
        virtual void SetErrorCode(DataErrorCode ec) = 0;

        /// @brief 获取response的size
        /// @return
        virtual int32_t GetSize() const = 0;

        // /// @brief 将string反序列化为response
        // /// @param req_str
        // /// @return
        // virtual bool ParseFromString(const std::string &resp_str) = 0;

        // /// @brief 将response序列化为string
        // /// @return
        // virtual std::string SerializeToString() = 0;
    };

    using RequestPtr = std::shared_ptr<IRequest>;
    using ResponsePtr = std::shared_ptr<IResponse>;
}