/// @brief JSON-RPC 2.0 编解码器
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <interface/i_coder.h>
#include <net/net_data.h>
#include <string>
#include <string_view>
#include <optional>
#include <variant>

namespace jl
{

/// @brief JSON-RPC 2.0 错误结构
struct JsonRpcError
{
    int32_t code = 0;
    std::string message;
    std::string data;
};

/// @brief JSON-RPC 2.0 请求/响应解析结果
/// Request: {"jsonrpc": "2.0", "method": "service.method", "params": {...}, "id": "1"}
/// Success Response: {"jsonrpc": "2.0", "result": {...}, "id": "1"}
/// Error Response: {"jsonrpc": "2.0", "error": {"code": -32600, "message": "Invalid Request"}, "id": "1"}

struct JsonRpcRequest
{
    std::string jsonrpc = "2.0";
    std::string method;
    std::string params;  // 序列化的JSON字符串
    std::string id;
};

struct JsonRpcResponse
{
    std::string jsonrpc = "2.0";
    std::string result;  // 序列化的JSON字符串，错误时为空
    std::optional<JsonRpcError> error;
    std::string id;

    bool IsError() const { return error.has_value(); }
};

/// @brief JSON-RPC 2.0 Coder
/// 使用 nlohmann/json 库进行 JSON 序列化/反序列化
class JsonCoder : public Coder<JsonCoder>
{
public:
    /// @brief 编码请求。将 JsonRpcRequest 序列化为 JSON 字符串
    /// @param request
    /// @return JSON 字符串
    std::string EncodeRequest(const Request* request) ;

    /// @brief 解码请求。将 JSON 字符串解析为 Request
    /// @param req_str JSON-RPC 请求字符串
    /// @return 解析成功返回 Request，失败返回 nullptr
    Request* DecodeRequest(std::string_view req_str) ;

    /// @brief 编码响应。将 JsonRpcResponse 序列化为 JSON 字符串
    /// @param response
    /// @return JSON 字符串
    std::string EncodeResponse(const Response* response) ;

    /// @brief 解码响应。将 JSON 字符串解析为 Response
    /// @param resp_str JSON-RPC 响应字符串
    /// @return 解析成功返回 Response，失败返回 nullptr
    Response* DecodeResponse(std::string_view resp_str) ;

    /// @brief 解析 JSON-RPC 请求字符串
    /// @param req_str
    /// @return 成功返回 JsonRpcRequest，失败返回 nullptr
    static JsonRpcRequest* ParseJsonRpcRequest(std::string_view req_str);

    /// @brief 构建 JSON-RPC 成功响应
    /// @param id 请求 id
    /// @param result 结果（序列化后的 JSON 字符串）
    /// @return JSON 字符串
    static std::string BuildJsonRpcResponse(const std::string& id, const std::string& result);

    /// @brief 构建 JSON-RPC 错误响应
    /// @param id 请求 id
    /// @param error_code 错误码
    /// @param error_message 错误信息
    /// @return JSON 字符串
    static std::string BuildJsonRpcErrorResponse(const std::string& id, int32_t error_code, const std::string& error_message);
};

Coder<JsonCoder>& GetJsonCoder();

} // namespace jl
