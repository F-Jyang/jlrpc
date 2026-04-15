#include "pb_session.h"
#include <utils/easy_log.hpp>

namespace jl
{
    PbSession::PbSession(net::tcp::socket&& socket)
        : connection_(std::make_shared<TcpConnection>(std::move(socket), ConnectionState::kActived, kDefaultBufferSize)),
          state_(PbSessionState::kReadTotalLen)
    {
        // LOG_DEBUG << "New tcpConnection";
        ConnectionInfo info = connection_->GetConnectionInfo();
        session_id_ = info.hash();
        // std::stringstream oss;
        // oss << info_.local_ip << ":" << info_.local_port << "-" << info_.remote_ip << ":" << info_.remote_port << "-" << info_.protocol;
        // id_ = std::hash<std::string>{}(oss.str());
    }

    PbSession::PbSession(const ConnectionPtr& conn)
        :connection_(conn), 
        state_(PbSessionState::kReadTotalLen)
    {
		start_ = std::chrono::steady_clock::now();
        // LOG_DEBUG << "New tcpConnection";
        ConnectionInfo info = connection_->GetConnectionInfo();
        session_id_ = info.hash();
    }

    void PbSession::Start()
    {
        connection_->ReadLen(kTotalLenSize);
    }

    std::size_t PbSession::GetId() const
    {
        return session_id_;
    }

    void PbSession::SetRequestCallback(const PbRequestCallback &callback)
    {
        request_callback_ = callback;
        std::weak_ptr<PbSession> weak = shared_from_this();
        connection_->SetReadCallback(
            [weak](const ConnectionPtr &conn, const std::string &read_str)
            {
                PbSessionPtr self = weak.lock();
                if (!self)
                    return;
                // self->OnRequest(conn, buffer, bytes_transfered);
                if (self->state_ == PbSessionState::kReadTotalLen)
                {
                    if (read_str.size() != kTotalLenSize)
                    {
                        // log
                        assert(false);
                        // error
                        return;
                    }
                    std::size_t req_len = 0;
                    memcpy(&req_len, read_str.c_str(), kTotalLenSize);
                    self->connection_->ReadLen(req_len - kTotalLenSize);
                    self->state_ = PbSessionState::kReadRequest;
                }
                else if (self->state_ == PbSessionState::kReadRequest)
                {
                    Request* req_ptr = GetPbCoder().DecodeRequest(read_str);
                    self->request_callback_(self, req_ptr);
                    self->state_ = PbSessionState::kReadTotalLen;
                    delete req_ptr;
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

    void PbSession::WriteResponse(const Response* response)
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

    const asio::any_io_executor& PbSession::GetIoExecutor()
    {
        return connection_->GetIoExecutor();
    }

    void PbSession::Close()
    {
        connection_->Close();
    }

    PbSession::~PbSession()
    {
        auto now = std::chrono::steady_clock::now();
        int live_sec = std::chrono::duration_cast<std::chrono::seconds>(now - start_).count();
        LOG_DEBUG << "Session live: " << std::to_string(live_sec);
    }

    void PbSession::OnRequest(const ConnectionPtr &conn, const std::string &req_str)
    {
        if (state_ == PbSessionState::kReadTotalLen)
        {
            if (req_str.size() != kTotalLenSize)
            {
                // log
                assert(false);
                // error
                return;
            }
            std::size_t req_len = 0;
            memcpy(&req_len, req_str.c_str(), kTotalLenSize);
            connection_->ReadLen(req_len - kTotalLenSize);
            state_ = PbSessionState::kReadRequest;
        }
        else if (state_ == PbSessionState::kReadRequest)
        {
            Request* req_ptr = GetPbCoder().DecodeRequest(req_str);
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
