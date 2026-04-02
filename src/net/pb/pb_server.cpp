#include "pb_server.h"
#include <net/pb/pb_data.h>

namespace jl
{
    PbServer::PbServer(asio::io_context& ioct, const std::string& ip, unsigned short port) :
        tcp_server_(std::make_unique<Server>(ioct, ip, port)),
        dispacher_(std::make_unique<PbRpcDispatcher>()),
        pb_coder_(std::make_unique<PbCoder>())
    {
        static int id = 0;
        tcp_server_->SetConnEstablishCallback(
            [&](net::tcp::socket &&socket)
            {
                // TODO: generate session id, hash 4 元组？？？
                int64_t session_id = 12345;
                SessionPtr session_ptr = std::make_shared<PbSession>(id++, std::move(socket));
                session_ptr->SetReadCallback(
                    [&](const SessionPtr &ptr, asio::streambuf& buffer)
                    {
                        this->OnSessionRead(ptr, buffer);
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
                session_map_.emplace(std::make_pair(id, session_ptr));
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
        int64_t id = session_ptr->GetId();
        if (session_map_.find(id) != session_map_.end())
        {
            session_map_.erase(id);
        }
    }

    void PbServer::OnSessionRead(const SessionPtr &ptr, asio::streambuf& buffer)
    {
        RequestPtr req_ptr = pb_coder_->DecodeRequest(buffer);
        if (!req_ptr)
        {
            // log
            ptr->Close(); // or send msg_id = -1 and error code ???
            return;
        }
        ResponsePtr resp_ptr = dispacher_->Dispatch(req_ptr);
        std::string resp_str = pb_coder_->EncodeResponse(resp_ptr);
        ptr->Write(resp_str);
        // read next rpc request
        ptr->Read();
    }

    void PbServer::OnSessionWrite(const SessionPtr &ptr, std::size_t bytes_transferred)
    {
        // log
    }
}
