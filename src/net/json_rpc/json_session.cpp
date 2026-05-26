#include "json_session.h"
#include <utils/easy_log.hpp>
#include <sstream>

namespace jl
{

JsonSession::JsonSession(const ConnectionPtr& conn)
    : connection_(conn),
      state_(JsonSessionState::kReadHttpHeader)
{
    start_ = std::chrono::steady_clock::now();
    ConnectionInfo info = connection_->GetConnectionInfo();
    session_id_ = info.hash();
}

void JsonSession::Start()
{
    LOG_DEBUG << "JsonSession::Start called, id=" << std::to_string(session_id_);
    ReadHttpHeader();
}

std::size_t JsonSession::GetId() const
{
    return session_id_;
}

void JsonSession::SetRequestCallback(const JsonRequestCallback& callback)
{
    request_callback_ = callback;
}

void JsonSession::WriteResponse(const Response* response)
{
    std::string json_resp = GetJsonCoder().EncodeResponse(response);   
    std::string http_resp = HttpCoder::BuildOkResponse(json_resp);
    LOG_DEBUG << "BuildOkResponse complete, http_resp size=" + std::to_string(http_resp.size()) << ", http_body size=" + std::to_string(json_resp.size());
    connection_->AsyncWrite(http_resp);
}

void JsonSession::SetResponseCallback(const JsonResponseCallback& callback)
{
    response_callback_ = callback;
    std::weak_ptr<ISession> weak = shared_from_this();
    connection_->SetWriteCallback(
        [weak](const ConnectionPtr& conn, std::size_t bytes_transferred, const std::error_code& ec)
        {
            SessionPtr self = weak.lock();
            if (!self)
                return;
            self->OnResponse(self, bytes_transferred, ec);
        });
}

void JsonSession::SetCloseCallback(const JsonSessionCloseCallback& callback)
{
    close_callback_ = callback;
    std::weak_ptr<ISession> weak = shared_from_this();
    connection_->SetCloseCallback(
        [weak](const ConnectionPtr& conn, const std::error_code& ec)
        {
            SessionPtr self = weak.lock();
            if (!self)
                return;
            self->OnSessionCloseCallback(self, ec);
        });
}

const asio::any_io_executor& JsonSession::GetIoExecutor()
{
    return connection_->GetIoExecutor();
}

void JsonSession::Close()
{
    connection_->Close();
}

JsonSession::~JsonSession()
{
    auto now = std::chrono::steady_clock::now();
    int live_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_).count();
    LOG_DEBUG << "JsonSession live: " << std::to_string(live_sec);
}

void JsonSession::ReadHttpHeader()
{
    LOG_DEBUG << "JsonSession::ReadHttpHeader called, id=" << std::to_string(session_id_);
    state_ = JsonSessionState::kReadHttpHeader;
    http_req_data_.clear();

    std::weak_ptr<ISession> weak = shared_from_this();
    connection_->SetReadCallback(
        [weak](const ConnectionPtr& conn, std::string_view read_str, const std::error_code& ec)
        {
            SessionPtr self = weak.lock();
            if (!self)
                return;
            self->OnRead(self, read_str, ec);
        });

    // 读取直到遇到 \r\n\r\n
    LOG_DEBUG << "Calling AsyncReadUtil, id=" << std::to_string(session_id_);
    connection_->AsyncReadUtil("\r\n\r\n");
}

void JsonSession::ReadHttpBody(size_t body_size)
{
    state_ = JsonSessionState::kReadHttpBody;
    std::weak_ptr<ISession> weak = shared_from_this();
    connection_->SetReadCallback(
        [weak](const ConnectionPtr& conn, std::string_view read_str, const std::error_code& ec)
        {
            SessionPtr self = weak.lock();
            if (!self)
                return;
            self->OnRead(self, read_str, ec);
        });
    LOG_DEBUG << "Read HTTP body size: " << std::to_string(body_size);
    connection_->AsyncReadLen(body_size);
}

void JsonSession::ProcessRequest(const std::string& body)
{
    LOG_DEBUG << "ProcessRequest called, body size: " + std::to_string(body.size());
    Request* req_ptr = GetJsonCoder().DecodeRequest(body);
    LOG_DEBUG << "DecodeRequest returned: " << (req_ptr ? "valid" : "null");
    if (request_callback_)
    {
        request_callback_(shared_from_this(), req_ptr, std::error_code());
    }
    if (req_ptr)
        delete req_ptr;
}

void JsonSession::OnRead(const SessionPtr& session, std::string_view read_str, const std::error_code& ec)
{
    LOG_DEBUG << "JsonSession::OnRead called, state=" + std::to_string(static_cast<int>(session->state_.load())) + ", data_size=" + std::to_string(read_str.size());
    if (ec)
    {
        LOG_DEBUG << __FUNCTION__ << ": " << ec.message();
        if (request_callback_)
            request_callback_(session, nullptr, ec);
        return;
    }

    if (session->state_ == JsonSessionState::kReadHttpHeader)
    {
        session->http_req_data_.append(read_str.data(), read_str.size());
        LOG_DEBUG << "http_req_data_ size after append: " + std::to_string(session->http_req_data_.size());

        // 尝试解析 HTTP header
        std::unique_ptr<HttpRequest> req(HttpCoder::DecodeRequest(session->http_req_data_));
        if (!req)
        {
            // 继续读取
            return;
        }

        // 检查 Content-Length
        std::string_view content_length = req->GetHeader("Content-Length");
        size_t body_size = 0;
        if (!content_length.empty())
        {
            body_size = std::stoul(std::string(content_length));
        }

        if (body_size > 0)
        {
            // req->body 包含HTTP头结束后到当前位置的所有数据
            // 可能是完整的body，也可能是部分body（如果ASIO读取了多余的字节）
            size_t already_read = req->body.size();
            if (already_read >= body_size)
            {
                // 我们已经读取了完整的body，直接处理
                std::string body_data = req->body.substr(0, body_size);
                // 将多余的字节保存到http_req_data_，但实际上通常不会有多余字节
                session->http_req_data_ = req->body.substr(body_size);
                session->ProcessRequest(body_data);
                session->ReadHttpHeader();
            }
            else
            {
                // 还需要继续读取body
                session->http_req_data_ = req->body;
                session->ReadHttpBody(body_size - already_read);
            }
        }
        else
        {
            // 没有 body，直接处理请求
            session->ProcessRequest(req->body);
            session->ReadHttpHeader();
        }
    }
    else if (session->state_ == JsonSessionState::kReadHttpBody)
    {
        session->http_req_data_.append(read_str.data(), read_str.size());
        session->ProcessRequest(session->http_req_data_);
        session->ReadHttpHeader();
    }
}

void JsonSession::OnResponse(const SessionPtr& session, std::size_t bytes_transferred, const std::error_code& ec)
{
    if (response_callback_)
        response_callback_(session, bytes_transferred, ec);
}

} // namespace jl
