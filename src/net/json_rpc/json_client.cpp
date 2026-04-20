#include "json_client.h"
#include <cstring>

namespace jl
{

JsonClient::JsonClient(asio::io_context& ioct, std::size_t max_buffer_size)
    : ioct_(ioct),
      timeout_(30)
{
    connection_ = std::make_shared<TcpConnection>(ioct, max_buffer_size);
    connection_->SetTimeoutCallback(
        [&](const ConnectionPtr& conn, const std::error_code& ec)
        {
            LOG_DEBUG << "JsonClient timeout";
            conn->Cancel();
        });
}

bool JsonClient::Connect(const std::string& ip, unsigned short port)
{
    return connection_->Connect(ip, port);
}

bool JsonClient::IsConnected()
{
    return connection_->IsConnected();
}

bool JsonClient::SendRequest(const Request* req_ptr)
{
    // 构建 JSON-RPC 请求
    std::string json_req = GetJsonCoder().EncodeRequest(req_ptr);
    LOG_DEBUG << "SendRequest: json_req=" + json_req << " size=" << std::to_string(json_req.size());

    // 构建 HTTP POST 请求
    HttpRequest http_req;
    http_req.method = "POST";
    http_req.uri = "/";
    http_req.version = "HTTP/1.1";
    http_req.body = json_req;

    HttpResponse http_resp;
    http_resp.status_code = 200;
    http_resp.status_message = "OK";
    http_resp.SetContentType("application/json");
    http_resp.SetBody(json_req);

    // 构建原始 HTTP 请求字符串
    std::stringstream ss;
    ss << "POST / HTTP/1.1\r\n";
    ss << "Host: localhost\r\n";
    ss << "Content-Type: application/json\r\n";
    ss << "Content-Length: " << json_req.size() << "\r\n";
    ss << "\r\n";
    ss << json_req;

    std::string http_req_str = ss.str();
    LOG_DEBUG << "SendRequest: http_req_str size=" + std::to_string(http_req_str.size());

    std::error_code ec;
    std::size_t n = connection_->SyncWrite(http_req_str, timeout_, ec);
    if (ec)
    {
        LOG_DEBUG << "SendRequest error: " << ec.message();
        return false;
    }
    LOG_DEBUG << "SendRequest: wrote " + std::to_string(n) + " bytes";
    return n == http_req_str.size();
}

Response* JsonClient::ReadResponse()
{
    std::error_code ec;

    LOG_DEBUG << "ReadResponse: waiting for header";
    // 读取 HTTP header
    std::string header_data = connection_->SyncReadUtil("\r\n\r\n", timeout_, ec);
    if (ec)
    {
        LOG_DEBUG << "ReadResponse header error: " << ec.message();
        return nullptr;
    }
    LOG_DEBUG << "ReadResponse: got header, size=" + std::to_string(header_data.size());

    // 解析 HTTP header
    std::unique_ptr<HttpResponse> resp(HttpCoder::DecodeResponse(header_data));
    if (!resp)
    {
        LOG_DEBUG << "ReadResponse: failed to decode header";
        return nullptr;
    }

    // 获取 Content-Length
    std::string_view content_length = resp->GetHeader("Content-Length");
    if (content_length.empty())
    {
        LOG_DEBUG << "ReadResponse: no Content-Length";
        return nullptr;
    }

    size_t body_size = std::stoul(std::string(content_length));
    LOG_DEBUG << "ReadResponse: body_size=" + std::to_string(body_size);

    // 读取 body
    std::string body_data = connection_->SyncReadLen(body_size, timeout_, ec);
    if (ec)
    {
        LOG_DEBUG << "ReadResponse body error: " << ec.message();
        return nullptr;
    }
    LOG_DEBUG << "ReadResponse: got body, size=" + std::to_string(body_data.size());

    // 解析 JSON-RPC 响应
    Response* resp_ptr = GetJsonCoder().DecodeResponse(body_data);
    return resp_ptr;
}

void JsonClient::Close()
{
    connection_->Close();
}

void JsonClient::SetTimeout(int secs)
{
    timeout_ = secs;
}

void JsonClient::OnTimeout(const std::error_code& ec)
{
    if (!ec)
    {
        LOG_DEBUG << __FUNCTION__ << ":" << ec.message();
        connection_->Cancel();
    }
    else
    {
        LOG_DEBUG << __FUNCTION__ << ":" << ec.message();
    }
}

} // namespace jl
