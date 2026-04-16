#include "pb_session.h"
#include <utils/easy_log.hpp>

namespace jl
{
    // PbSession::PbSession(asio::io_context& ioct, net::tcp::socket &&socket)
    //     : connection_(std::make_shared<TcpConnection>(ioct,std::move(socket), kDefaultBufferSize)),
    //       state_(PbSessionState::kReadTotalLen)
    // {
    //     // LOG_DEBUG << "New tcpConnection";
    //     ConnectionInfo info = connection_->GetConnectionInfo();
    //     session_id_ = info.hash();
    //     // std::stringstream oss;
    //     // oss << info_.local_ip << ":" << info_.local_port << "-" << info_.remote_ip << ":" << info_.remote_port << "-" << info_.protocol;
    //     // id_ = std::hash<std::string>{}(oss.str());
    // }

    PbSession::PbSession(const ConnectionPtr &conn)
        : connection_(conn),
          state_(PbSessionState::kReadTotalLen)
    {
        start_ = std::chrono::steady_clock::now();
        // LOG_DEBUG << "New tcpConnection";
        ConnectionInfo info = connection_->GetConnectionInfo();
        session_id_ = info.hash();
    }

    void PbSession::Start()
    {
        connection_->AsyncReadLen(kTotalLenSize);
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
            [weak](const ConnectionPtr &conn, std::string_view read_str, const std::error_code &ec)
            {
                PbSessionPtr self = weak.lock();
                if (!self)
                    return;
                self->OnRequest(self, read_str, ec);
                // if (self->state_ == PbSessionState::kReadTotalLen)
                // {
                //     if (read_str.size() != kTotalLenSize)
                //     {
                //         // log
                //         assert(false);
                //         // error
                //         return;
                //     }
                //     std::size_t req_len = 0;
                //     memcpy(&req_len, read_str.data(), kTotalLenSize);
                //     self->connection_->ReadLen(req_len - kTotalLenSize);
                //     self->state_ = PbSessionState::kReadRequest;
                // }
                // else if (self->state_ == PbSessionState::kReadRequest)
                // {
                //     Request *req_ptr = GetPbCoder().DecodeRequest(read_str);
                //     self->request_callback_(self, req_ptr, ec);
                //     self->state_ = PbSessionState::kReadTotalLen;
                //     delete req_ptr;
                // }
                // else
                // {
                //     // log
                //     assert(false);
                //     // error
                //     return;
                // }
            });
    }

    void PbSession::WriteResponse(const Response *response)
    {
        std::string resp_str = GetPbCoder().EncodeResponse(response);
        connection_->AsyncWrite(resp_str);
    }

    void PbSession::SetResponseCallback(const PbResponseCallback &callback)
    {
        response_callback_ = callback;
        std::weak_ptr<PbSession> weak = shared_from_this();
        connection_->SetWriteCallback(
            [weak](const ConnectionPtr &conn, std::size_t bytes_transferred, const std::error_code &ec)
            {
                PbSessionPtr self = weak.lock();
                if (!self)
                    return;
                self->OnResponse(self, bytes_transferred, ec);
                // self->response_callback_(self, bytes_transferred, ec);
            });
    }

    void PbSession::SetCloseCallback(const PbSessionCloseCallback &callback)
    {
        close_callback_ = callback;
        std::weak_ptr<PbSession> weak = shared_from_this();
        connection_->SetCloseCallback(
            [weak](const ConnectionPtr &conn, const std::error_code &ec)
            {
                PbSessionPtr self = weak.lock();
                if (!self || !self->close_callback_)
                    return;
                self->close_callback_(self, ec);
            });
    }

    const asio::any_io_executor &PbSession::GetIoExecutor()
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

    void PbSession::OnRequest(const PbSessionPtr &session, std::string_view req_str, const std::error_code &ec)
    {
        if (ec)
        {
            // log error
            LOG_DEBUG << __FUNCTION__ << ": " << ec.message();
            session->request_callback_(session, nullptr, ec);
            return;
        }
        if (session->state_ == PbSessionState::kReadTotalLen)
        {
            if (req_str.size() != kTotalLenSize)
            {
                // log
                assert(false);
                // error
                return;
            }
            std::size_t req_len = 0;
            memcpy(&req_len, req_str.data(), kTotalLenSize);
            session->connection_->AsyncReadLen(req_len - kTotalLenSize);
            session->state_ = PbSessionState::kReadRequest;
        }
        else if (session->state_ == PbSessionState::kReadRequest)
        {
            Request *req_ptr = GetPbCoder().DecodeRequest(req_str);
            session->request_callback_(session, req_ptr, ec);
            session->state_ = PbSessionState::kReadTotalLen;
            delete req_ptr;
        }
        else
        {
            // log
            assert(false);
            // error
            return;
        }
    }

    void PbSession::OnResponse(const PbSessionPtr &session, std::size_t bytes_transferred, const std::error_code &ec)
    {
        session->response_callback_(session, bytes_transferred, ec);
    }
}
