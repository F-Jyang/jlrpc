/// @brief HTTP 编解码器
/// @author Jyang.
/// @date 2026-4-19

#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <vector>

namespace jl
{

/// @brief HTTP 请求解析结果
struct HttpRequest
{
    std::string method;      // GET, POST, PUT, DELETE, etc.
    std::string uri;         // /path?query
    std::string version;     // HTTP/1.1
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;        // request body

    std::string_view GetHeader(const std::string& name) const;
    std::string_view GetBody() const { return body; }
};

/// @brief HTTP 响应
struct HttpResponse
{
    std::string version = "HTTP/1.1";
    int status_code = 200;
    std::string status_message = "OK";
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;

    std::string_view GetHeader(const std::string& name) const;
    void SetContentType(const std::string& ct);
    void SetBody(const std::string& b);
    std::string ToString() const;
};

/// @brief HTTP 编解码器
/// 用于解析HTTP请求和构建HTTP响应
class HttpCoder
{
public:
    /// @brief 解析HTTP请求
    /// @param data 原始数据
    /// @return 解析成功返回HttpRequest，失败返回nullptr
    static HttpRequest* DecodeRequest(std::string_view data);

    /// @brief 解析HTTP响应
    /// @param data 原始数据
    /// @return 解析成功返回HttpResponse，失败返回nullptr
    static HttpResponse* DecodeResponse(std::string_view data);

    /// @brief 构建HTTP响应
    /// @param response HTTP响应结构
    /// @return 序列化的HTTP响应字符串
    static std::string EncodeResponse(const HttpResponse& response);

    /// @brief 构建简单的200 OK响应
    static std::string BuildOkResponse(const std::string& body, const std::string& content_type = "application/json");

    /// @brief 构建错误响应
    static std::string BuildErrorResponse(int status_code, const std::string& message);
};

} // namespace jl
