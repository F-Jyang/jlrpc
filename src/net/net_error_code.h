#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace jl
{
    enum class NetErrorCode : int32_t
    {
        kNoError = 0,
        kServiceNotFound,  // 服务未注册
        kFunctionNotFound, // 函数未注册
        kInvaliadRequest,  // 请求解析异常
        kUnknown,          // 未知异常
    };

    const static std::unordered_map<NetErrorCode, std::string> kNetErrorMsgMap = {
        {NetErrorCode::kNoError, "No error."},
        {NetErrorCode::kServiceNotFound, "Service not found."},
        {NetErrorCode::kFunctionNotFound, "Function not found."},
        {NetErrorCode::kInvaliadRequest, "Invalid request."},
        {NetErrorCode::kUnknown, "Unknown error type."},
    };

    inline std::string GetPbErrorMsg(NetErrorCode error_type)
    {
        if (kNetErrorMsgMap.find(error_type) != kNetErrorMsgMap.end())
        {
            return kNetErrorMsgMap.find(error_type)->second;
        }
        return "Unknown error type.";
    }
}