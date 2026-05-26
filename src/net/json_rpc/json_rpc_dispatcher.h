/// @brief JSON-RPC Dispatcher (without protobuf dependency)
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <string>
#include <string_view>
#include <functional>
#include <unordered_map>
#include <net/net_data.h>
#include <nlohmann/json.hpp>

namespace jl
{

using json = nlohmann::json;

/// @brief JSON-RPC 方法处理函数类型
/// @param params JSON-RPC params (通常是 JSON 对象或数组)
/// @param error 错误信息输出
/// @return 成功返回 result JSON，失败时返回空 json
using JsonRpcMethodHandler = std::function<json(const json& params, NetErrorCode& error)>;

/// @brief JSON-RPC 服务分发器
/// 使用 std::function 注册方法，无需 protobuf 依赖
/// Request/Response 复用 net_data.h 中的定义:
///   - Request.GetServiceFullName() -> method
///   - Request.GetParam() -> params (JSON string)
///   - Request.GetMsgId() -> id
///   - Response.SetResult() -> result (JSON string)
///   - Response.SetErrorCode() -> error code
class JsonRpcDispatcher
{
public:
    /// @brief 注册方法处理器
    /// @param method_name 方法名，格式 "service.method" 或纯方法名
    /// @param handler 方法处理函数
    void RegisterMethod(const std::string& method_name, JsonRpcMethodHandler handler);

    /// @brief 注销方法
    void UnregisterMethod(const std::string& method_name);

    /// @brief 分发请求
    /// @param req 请求对象
    /// @return 响应对象 (由调用者负责释放)
    Response* Dispatch(const Request* req);

    /// @brief 检查方法是否存在
    bool HasMethod(const std::string& method_name) const;

private:
    std::unordered_map<std::string, JsonRpcMethodHandler> method_handlers_;
};

// JSON-RPC 2.0 错误码 (映射到 NetErrorCode)
namespace JsonRpcErrorCode
{
    constexpr int32_t kParseError = -32700;
    constexpr int32_t kInvalidRequest = -32600;
    constexpr int32_t kMethodNotFound = -32601;
    constexpr int32_t kInvalidParams = -32602;
    constexpr int32_t kInternalError = -32603;
    constexpr int32_t kServerError = -32000;
}

inline NetErrorCode JsonRpcToNetError(int32_t json_rpc_code)
{
    switch (json_rpc_code)
    {
        case JsonRpcErrorCode::kInvalidRequest: return NetErrorCode::kInvaliadRequest;
        case JsonRpcErrorCode::kMethodNotFound: return NetErrorCode::kFunctionNotFound;
        case JsonRpcErrorCode::kInvalidParams: return NetErrorCode::kInvaliadRequest;
        default: return NetErrorCode::kUnknown;
    }
}

inline int32_t NetToJsonRpcError(NetErrorCode ec)
{
    switch (ec)
    {
        case NetErrorCode::kInvaliadRequest: return JsonRpcErrorCode::kInvalidRequest;
        case NetErrorCode::kFunctionNotFound: return JsonRpcErrorCode::kMethodNotFound;
        default: return JsonRpcErrorCode::kServerError;
    }
}

} // namespace jl
