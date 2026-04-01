#include "pb_server.h"
#include <net/pb/pb_data.h>

namespace jl
{
    PbServer::PbServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(std::make_unique<Server>(ioct, ip, port)),
                                                                                             dispacher_(std::make_unique<PbRpcDispatcher>())
    {
        tcp_server_->SetConnEstablishCallback(
            [&](net::tcp::socket &&socket)
            {
                // TODO: generate session id, hash 4 元组？？？
                int64_t session_id = 12345;
                SessionPtr session_ptr = std::make_shared<PbSession>(session_id, std::move(socket));
                // TODO: set session callback
                session_ptr->SetReadCallback(
                    [&](const SessionPtr &ptr, const std::shared_ptr<IRequest> &req_ptr)
                    {
                        this->OnSessionRead(ptr, req_ptr);
                    });

                session_ptr->SetWriteCallback(
                    [&](const SessionPtr &ptr, std::size_t bytes_transferred)
                    {
                        this->OnSessionWrite(ptr, bytes_transferred);
                    });

                session_ptr->SetCloseCallback(
                    [&](const SessionPtr &ptr)
                    {
                        this->OnSessionClose(ptr);
                    });
                std::lock_guard<std::mutex> lock(session_mutex_);
                session_map_.emplace(std::make_pair(session_id, session_ptr));
            });
    }

    void PbServer::Start()
    {
        tcp_server_->DoAwaitStop();
        tcp_server_->Start();
    }

    void PbServer::Stop()
    {
        tcp_server_->Stop();
    }

    void PbServer::OnSessionClose(const SessionPtr &session_ptr)
    {
        // log
        std::lock_guard<std::mutex> lock(session_mutex_);
        if (session_map_.find(session_ptr->GetId()) != session_map_.end())
        {
            session_map_.erase(session_ptr->GetId());
        }
    }

    void PbServer::OnSessionRead(const SessionPtr &ptr, const std::shared_ptr<IRequest> &req_ptr)
    {
        dispacher_->Dispatch(ptr,req_ptr);
    }

    void PbServer::OnSessionWrite(const SessionPtr &ptr, std::size_t bytes_transferred)
    {
    }
}
