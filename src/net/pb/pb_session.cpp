#include "pb_session.h"
#include <utils/easy_log.hpp>

namespace jl
{
    PbSession::PbSession(net::tcp::socket &&socket)
        : connection_(std::make_shared<TcpConnection>(std::move(socket))),
        state_(PbSessionState::kReadTotalLen)
    {
        LOG_DEBUG << "New tcpConnection";
        ConnectionInfo info = connection_->GetConnectionInfo();
        session_id_ = info.hash();
        // std::stringstream oss;
        // oss << info_.local_ip << ":" << info_.local_port << "-" << info_.remote_ip << ":" << info_.remote_port << "-" << info_.protocol;
        // id_ = std::hash<std::string>{}(oss.str());
    }

    void PbSession::Start()
    {
        connection_->ReadLen(kTotalLenSize);
    }

    std::size_t PbSession::GetId() const
    {
        return std::size_t();
    }

    void PbSession::SetRequestCallback(const PbRequestCallback &callback)
    {
        request_callback_ = callback;
        std::weak_ptr<PbSession> weak = shared_from_this();
        connection_->SetReadCallback(
            [weak](const ConnectionPtr &conn, asio::streambuf &buffer, size_t bytes_transfered)
            {
                PbSessionPtr self = weak.lock();
                if (!self)
                    return;
                // self->OnRequest(conn, buffer, bytes_transfered);
                if (self->state_ == PbSessionState::kReadTotalLen)
                {
                    if (bytes_transfered != kTotalLenSize)
                    {
                        // log
                        assert(false);
                        // error
                        return;
                    }
                    std::size_t req_len = 0;
                    std::istream is(&buffer);
                    is.read(reinterpret_cast<char *>(&req_len), kTotalLenSize);
                    self->connection_->ReadLen(req_len - kTotalLenSize);
                    self->state_ = PbSessionState::kReadRequest;
                }
                else if (self->state_ == PbSessionState::kReadRequest)
                {
                    RequestPtr req_ptr = GetPbCoder().DecodeRequest(buffer, bytes_transfered);
                    self->request_callback_(self, req_ptr);
                    self->state_ = PbSessionState::kReadTotalLen;
                }
                else
                {
                    // log
                    assert(false);
                    // error
                    return;
                }
            });
    }

    void PbSession::WriteResponse(const ResponsePtr &response)
    {
        std::string resp_str = GetPbCoder().EncodeResponse(response);
        connection_->Write(resp_str);
    }

    void PbSession::SetResponseCallback(const PbResponseCallback &callback)
    {
        response_callback_ = callback;
        std::weak_ptr<PbSession> weak = shared_from_this();
        connection_->SetWriteCallback(
            [weak](const ConnectionPtr &conn, std::size_t bytes_transferred)
            {
                PbSessionPtr self = weak.lock();
                if (!self)
                    return;
                self->response_callback_(self, bytes_transferred);
            });
    }

    void PbSession::SetCloseCallback(const PbSessionCloseCallback &callback)
    {
        close_callback_ = callback;
        std::weak_ptr<PbSession> weak = shared_from_this();
        connection_->SetCloseCallback(
            [weak](const ConnectionPtr &conn)
            {
                PbSessionPtr self = weak.lock();
                if (!self || !self->close_callback_)
                    return;
                self->close_callback_(self);
            });
    }

    std::size_t PbSession::GetTimeout() const
    {
        return timeout_;
    }

    void PbSession::SetTimeout(std::size_t timeout)
    {
        timeout_ = timeout;
    }

    void PbSession::Close()
    {
        connection_->Close();
    }

    PbSession::~PbSession()
    {
        LOG_DEBUG << "Session close";
    }

    void PbSession::OnRequest(const ConnectionPtr &conn, asio::streambuf &buffer, size_t bytes_transfered)
    {
        if (state_ == PbSessionState::kReadTotalLen)
        {
            if (bytes_transfered != kTotalLenSize)
            {
                // log
                assert(false);
                // error
                return;
            }
            std::size_t req_len = 0;
            std::istream is(&buffer);
            is.read(reinterpret_cast<char *>(&req_len), kTotalLenSize);
            connection_->ReadLen(req_len);
        }
        else if (state_ == PbSessionState::kReadRequest)
        {
            RequestPtr req_ptr = GetPbCoder().DecodeRequest(buffer, bytes_transfered);
            request_callback_(shared_from_this(), req_ptr);
        }
        else
        {
            // log
            assert(false);
            // error
            return;
        }
    }

    void PbSession::OnResponse(const ConnectionPtr &conn, std::size_t bytes_transferred)
    {
        response_callback_(shared_from_this(), bytes_transferred);
    }
}
