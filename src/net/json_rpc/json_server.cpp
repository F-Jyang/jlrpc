#include "json_server.h"
#include <net/net_data.h>
#include <utils/easy_log.hpp>

namespace jl
{

JsonServer::JsonServer(asio::io_context& main_ioct, const std::string& ip, unsigned short port,
                       std::size_t thread_size)
    : tcp_server_(std::make_unique<Server>(main_ioct, ip, port, thread_size)),
      dispatcher_(std::make_unique<JsonRpcDispatcher>()),
      timer_wheel_(std::make_unique<TimerWheel>(main_ioct))
{
    tcp_server_->SetConnEstablishCallback(
        [&](const ConnectionPtr& conn)
        {
            JsonSessionPtr session_ptr = std::make_shared<JsonSession>(conn);
            session_ptr->SetTimeout(30);
            TimerEventPtr event_ptr = std::make_shared<TimerEvent>(
                session_ptr,
                [session_weak = session_ptr->weak_from_this()]()
                {
                    LOG_DEBUG << "JsonSession timeout event";
                    SessionPtr session_ptr = session_weak.lock();
                    if (session_ptr)
                    {
                        LOG_DEBUG << "JsonSession timeout close";
                        session_ptr->Close();
                    }
                });
            TimerEventWeak event_weak(event_ptr);
            session_ptr->SetContext(TimerEventWeak(event_ptr));
            timer_wheel_->AddTimerEvent(event_ptr);
            std::size_t session_id = session_ptr->GetId();
            LOG_DEBUG << "Accept new JsonSession: " << std::to_string(session_id);

            session_ptr->SetRequestCallback(
                [&](const JsonSessionPtr& session, const Request* request, const std::error_code& ec)
                {
                    this->OnSessionRead(session, request, ec);
                });

            session_ptr->SetResponseCallback(
                [&](const JsonSessionPtr& session, std::size_t bytes_transferred, const std::error_code& ec)
                {
                    this->OnSessionWrite(session, bytes_transferred, ec);
                });

            session_ptr->SetCloseCallback(
                [&](const JsonSessionPtr& session, const std::error_code& ec)
                {
                    this->OnSessionClose(session, ec);
                });

            session_ptr->Start();

            std::lock_guard<std::mutex> lock(session_mutex_);
            session_map_.emplace(std::make_pair(session_id, session_ptr));
        });
}

void JsonServer::RegisterMethod(const std::string& method_name, JsonRpcMethodHandler handler)
{
    dispatcher_->RegisterMethod(method_name, std::move(handler));
}

void JsonServer::UnregisterMethod(const std::string& method_name)
{
    dispatcher_->UnregisterMethod(method_name);
}

void JsonServer::Start()
{
    timer_wheel_->Start();
    tcp_server_->Start();
}

void JsonServer::Stop()
{
    tcp_server_->Stop();
}

void JsonServer::OnSessionClose(const JsonSessionPtr& session_ptr, const std::error_code& ec)
{
    LOG_DEBUG << "JsonSession close";
    int64_t id = session_ptr->GetId();
    std::lock_guard<std::mutex> lock(session_mutex_);
    if (session_map_.find(id) != session_map_.end())
    {
        session_map_.erase(id);
    }
}

void JsonServer::OnSessionRead(const JsonSessionPtr& session, const Request* request, const std::error_code& ec)
{
    LOG_DEBUG << "JsonServer::OnSessionRead called, ec=" << ec.message() << ", request=" << (request ? "valid" : "null");
    if (ec)
    {
        session->Close();
        return;
    }

    if (!request)
    {
        session->Close();
        return;
    }

    LOG_DEBUG << "Dispatching request, method=" << std::string(request->GetServiceFullName());
    Response* response = dispatcher_->Dispatch(request);
    LOG_DEBUG << "Dispatch complete, response=" << (response ? "valid" : "null");

    // Refresh timer
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

void JsonServer::OnSessionWrite(const JsonSessionPtr& session, std::size_t bytes_transferred, const std::error_code& ec)
{
    if (ec)
    {
        session->Close();
    }
}

} // namespace jl
