#include "pb_server.h"
#include <net/net_data.h>
#include <utils/easy_log.hpp>

namespace jl
{
    PbServer::PbServer(asio::io_context &main_ioct, const std::string &ip, unsigned short port, std::size_t thread_size) : tcp_server_(std::make_unique<Server>(main_ioct, ip, port, thread_size)),
                                                                                                                           dispatcher_(std::make_unique<PbRpcDispatcher>())
    {
        tcp_server_->SetConnEstablishCallback(
            [&](net::tcp::socket &&socket)
            {
                PbSessionPtr session_ptr = std::make_shared<PbSession>(std::move(socket));
                std::size_t session_id = session_ptr->GetId();
                LOG_DEBUG << "Accept new session: " << std::to_string(session_id);
                session_ptr->SetRequestCallback(
                    [&](const PbSessionPtr &session, const RequestPtr &request)
                    {
                        this->OnSessionRead(session, request);
                    });
                session_ptr->SetResponseCallback(
                    [&](const PbSessionPtr &session, std::size_t bytes_transferred)
                    {
                        this->OnSessionWrite(session, bytes_transferred);
                    });
                session_ptr->SetCloseCallback(
                    [&](const PbSessionPtr &session)
                    {
                        this->OnSessionClose(session);
                    });
                session_ptr->Start(); // 开始接收
                std::lock_guard<std::mutex> lock(session_mutex_);
                session_map_.emplace(std::make_pair(session_id, session_ptr));
            });
    }

    void PbServer::RegisterSerivce(const std::string &service_name, const PbServicePtr &service_ptr)
    {
        dispatcher_->RegisterService(service_name, service_ptr);
    }

    void PbServer::UnRegisterService(const std::string &service_name)
    {
        dispatcher_->UnRegisterService(service_name);
    }

    void PbServer::Start()
    {
        tcp_server_->Start();
    }

    void PbServer::Stop()
    {
        tcp_server_->Stop();
    }

    void PbServer::OnSessionClose(const PbSessionPtr &session_ptr)
    {
        LOG_DEBUG << "Session close";
        int64_t id = session_ptr->GetId();
        std::lock_guard<std::mutex> lock(session_mutex_);
        if (session_map_.find(id) != session_map_.end())
        {
            session_map_.erase(id);
        }
    }

    void PbServer::OnSessionRead(const PbSessionPtr &session, const RequestPtr &request)
    {
        // RequestPtr req_ptr = pb_coder_->DecodeRequest(buffer,bytes_transfered);
        if (!request)
        {
            std::string err_msg = std::to_string(session->GetId()) + " send invalid request.";
            LOG_DEBUG << err_msg;
            session->Close(); // or send msg_id = -1 and error code ???
            return;
        }
        ResponsePtr resp_ptr = dispatcher_->Dispatch(request);
        if (resp_ptr->GetMsgId() == "HEARTBEAT")
        {
            // TODO：refresh timeout
            session->SetTimeout(111);
        }
        // std::string resp_str = pb_coder_->EncodeResponse(resp_ptr);
        session->WriteResponse(resp_ptr);
        // read next rpc request
    }

    void PbServer::OnSessionWrite(const PbSessionPtr &session, std::size_t bytes_transferred)
    {
        assert(session);
        LOG_DEBUG << "Session write " << std::to_string(bytes_transferred);
        session->Start();
    }
}
