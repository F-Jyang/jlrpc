#include "pb_server.h"
#include <net/net_data.h>
#include <utils/easy_log.hpp>

namespace jl
{
    PbServer::PbServer(asio::io_context& main_ioct, const std::string& ip, unsigned short port, std::size_t thread_size)
        : tcp_server_(std::make_unique<Server>(main_ioct, ip, port, thread_size)),
        dispatcher_(std::make_unique<PbRpcDispatcher>()),
        timer_wheel_(std::make_unique<TimerWheel>(main_ioct))
    {
        tcp_server_->SetConnEstablishCallback(
            [&](net::tcp::socket &&socket)
            {
                ConnectionPtr conn = std::make_shared<TcpConnection>(std::move(socket), kDefaultBufferSize);
                PbSessionPtr session_ptr = std::make_shared<PbSession>(conn);
                session_ptr->SetTimeout(30);
                TimerEventPtr event_ptr = std::make_shared<TimerEvent>(
                    session_ptr,
                    [session_weak = session_ptr->weak_from_this()]()
                    {
                        LOG_DEBUG << "OnSession timeout event";
                        SessionPtr session_ptr = session_weak.lock();
                        if (session_ptr)
                        {
							LOG_DEBUG << "Session timeout close";
                            session_ptr->Close();
                        }
                    });
                TimerEventWeak event_weak(event_ptr);
                session_ptr->SetContext(TimerEventWeak(event_ptr));
                timer_wheel_->AddTimerEvent(event_ptr);
                std::size_t session_id = session_ptr->GetId();
                LOG_DEBUG << "Accept new session: " << std::to_string(session_id);
                session_ptr->SetRequestCallback(
                    [&](const PbSessionPtr& session, const Request* request)
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
        timer_wheel_->Start();
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

    void PbServer::OnSessionRead(const PbSessionPtr &session, const Request* request)
    {
        // RequestPtr req_ptr = pb_coder_->DecodeRequest(buffer,bytes_transfered);
        if (!request)
        {
            std::string err_msg = std::to_string(session->GetId()) + " send invalid request.";
            LOG_DEBUG << err_msg;
            session->Close(); // or send msg_id = -1 and error code ??
            return;
        }
        Response* response = dispatcher_->Dispatch(request);
        // if (response->GetMsgId() == "HEARTBEAT")
        // {
        //     // TODO：refresh timeout
        // }
        TimerEventWeak event_weak = std::any_cast<TimerEventWeak>(session->GetContext());
        TimerEventPtr event_ptr = event_weak.lock();
        if (event_ptr)
        {
            event_ptr->RefreshTimeout(30);
            timer_wheel_->AddTimerEvent(event_ptr);
        }
        session->WriteResponse(response);
		session->Start();
		delete response;
    }

    void PbServer::OnSessionWrite(const PbSessionPtr &session, std::size_t bytes_transferred)
    {
        assert(session);
        // LOG_DEBUG << "Session write " << std::to_string(bytes_transferred);
    }
}
