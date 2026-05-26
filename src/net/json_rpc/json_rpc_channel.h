/// @brief JSON-RPC Channel (without protobuf dependency)
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <string>
#include <memory>
#include <functional>
#include <net/json_rpc/json_client.h>
#include <nlohmann/json.hpp>

namespace jl
{
using json = nlohmann::json;

/// @brief JSON-RPC 响应回调
using JsonRpcResponseCallback = std::function<void(const std::string& result, NetErrorCode ec)>;

/// @brief JSON-RPC Channel
/// 简化版的 RPC Channel，无需 protobuf 依赖
class JsonRpcChannel
{
public:
    JsonRpcChannel(asio::io_context& ioct, const std::string& ip, uint16_t port);

    /// @brief 同步调用 RPC 方法
    /// @param method 方法名
    /// @param params JSON 格式的参数
    /// @param timeout 超时时间(秒)
    /// @return 成功返回 result JSON 字符串，失败返回空字符串
    std::string CallMethod(const std::string& method, const std::string& params, int timeout = 5);

    /// @brief 异步调用 RPC 方法
    /// @param method 方法名
    /// @param params JSON 格式的参数
    /// @param callback 回调函数
    void AsyncCallMethod(const std::string& method, const std::string& params, JsonRpcResponseCallback callback);

    /// @brief 同步调用 RPC 方法
    /// @param method 方法名
    /// @param params JSON 格式的参数
    /// @param result 输出参数，解析后的结果
    /// @param ec 错误码
    /// @return 是否成功
    bool CallMethod(const std::string& method, const json& params, json& result, NetErrorCode& ec);

    void Close();

private:
    asio::io_context& ioct_;
    std::string ip_;
    uint16_t port_;
    JsonClientPtr client_;
};

} // namespace jl
