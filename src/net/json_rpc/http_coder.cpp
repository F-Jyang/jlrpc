#include "http_coder.h"
#include <algorithm>
#include <cstring>
#include <sstream>

namespace jl
{

std::string_view HttpRequest::GetHeader(const std::string& name) const
{
    for (const auto& h : headers)
    {
        if (h.first == name)
            return h.second;
    }
    return "";
}

std::string_view HttpResponse::GetHeader(const std::string& name) const
{
    for (const auto& h : headers)
    {
        if (h.first == name)
            return h.second;
    }
    return "";
}

void HttpResponse::SetContentType(const std::string& ct)
{
    for (auto& h : headers)
    {
        if (h.first == "Content-Type")
        {
            h.second = ct;
            return;
        }
    }
    headers.emplace_back("Content-Type", ct);
}

void HttpResponse::SetBody(const std::string& b)
{
    body = b;
    std::stringstream ss;
    ss << body.size();
    for (auto& h : headers)
    {
        if (h.first == "Content-Length")
        {
            h.second = ss.str();
            return;
        }
    }
    headers.emplace_back("Content-Length", ss.str());
}

std::string HttpResponse::ToString() const
{
    std::stringstream ss;
    ss << version << " " << status_code << " " << status_message << "\r\n";
    for (const auto& h : headers)
    {
        ss << h.first << ": " << h.second << "\r\n";
    }
    ss << "\r\n";
    ss << body;
    return ss.str();
}

HttpRequest* HttpCoder::DecodeRequest(std::string_view data)
{
    auto req = new HttpRequest();

    // 查找 header 和 body 的分隔位置 (\r\n\r\n)
    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string_view::npos)
    {
        return nullptr; // 还没收到完整的 header
    }

    // 解析 request line
    std::string_view header_part = data.substr(0, header_end);
    std::istringstream iss{std::string(header_part)};

    std::string line;
    if (!std::getline(iss, line))
    {
        return nullptr;
    }

    // 移除 \r
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::istringstream line_ss{line};
    line_ss >> req->method >> req->uri >> req->version;

    // 解析 headers
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            break;

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            // 去掉开头的空格
            while (!value.empty() && value[0] == ' ')
                value.erase(value.begin());
            req->headers.emplace_back(key, value);
        }
    }

    // 解析 body
    if (header_end + 4 < data.size())
    {
        req->body = std::string(data.substr(header_end + 4));
    }

    return req;
}

HttpResponse* HttpCoder::DecodeResponse(std::string_view data)
{
    auto resp = new HttpResponse();

    // 查找 header 和 body 的分隔位置 (\r\n\r\n)
    size_t header_end = data.find("\r\n\r\n");
    if (header_end == std::string_view::npos)
    {
        return nullptr; // 还没收到完整的 header
    }

    // 解析 status line: "HTTP/1.1 200 OK"
    std::string_view header_part = data.substr(0, header_end);
    std::istringstream iss{std::string(header_part)};

    std::string line;
    if (!std::getline(iss, line))
    {
        return nullptr;
    }

    // 移除 \r
    if (!line.empty() && line.back() == '\r')
        line.pop_back();

    std::istringstream line_ss{line};
    line_ss >> resp->version >> resp->status_code >> resp->status_message;

    // 解析 headers
    while (std::getline(iss, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.empty())
            break;

        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos)
        {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 1);
            // 去掉开头的空格
            while (!value.empty() && value[0] == ' ')
                value.erase(value.begin());
            resp->headers.emplace_back(key, value);
        }
    }

    // 解析 body
    if (header_end + 4 < data.size())
    {
        resp->body = std::string(data.substr(header_end + 4));
    }

    return resp;
}

std::string HttpCoder::EncodeResponse(const HttpResponse& response)
{
    return response.ToString();
}

std::string HttpCoder::BuildOkResponse(const std::string& body, const std::string& content_type)
{
    HttpResponse resp;
    resp.status_code = 200;
    resp.status_message = "OK";
    resp.SetContentType(content_type);
    resp.SetBody(body);
    return resp.ToString();
}

std::string HttpCoder::BuildErrorResponse(int status_code, const std::string& message)
{
    HttpResponse resp;
    resp.status_code = status_code;
    resp.status_message = message;
    resp.SetContentType("text/plain");
    resp.SetBody(message);
    return resp.ToString();
}

} // namespace jl
