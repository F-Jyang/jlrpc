#include "pb_server.h"
#include <net/net_data.h>
#include <utils/easy_log.hpp>
#include <asio/post.hpp>

namespace jl
{
    PbServer::PbServer(asio::io_context &main_ioct, const std::string &ip, unsigned short port, std::size_t thread_size)
        : main_ioct_(main_ioct),
          tcp_server_(std::make_unique<Server>(main_ioct, ip, port, thread_size)),
          dispatcher_(std::make_unique<PbRpcDispatcher>())
    //   timer_wheel_(std::make_unique<TimerWheel>(main_ioct))
    {
        tcp_server_->SetConnEstablishCallback(
            [&](const ConnectionPtr &conn)
            {
                PbSessionPtr session_ptr = std::make_shared<PbSession>(conn);
                session_ptr->SetTimeout(kDefaultSessionTimeout);
                // 在 socket_ptr 的线程中运行添加TimerEvent

                SessionTimerManager::PostTimerEvent( // 添加超时事件
                    session_ptr,
                    [session_weak = session_ptr->weak_from_this()]()
                    {
                        LOG_THREAD_ID("OnSession timeout event");
                        SessionPtr session_ptr = session_weak.lock();
                        if (session_ptr)
                        {
                            LOG_THREAD_ID("Session timeout close");
                            session_ptr->Close();

                            // 运行在同个线程的timerwheel中
                            // 如果 TimerWheel 运行与main_ioct中，需要将Close操作post到session_ptr所在线程
                            // 如果timerwheel使用的是GetLocalTimerWheel，运行在与session_ptr的同个线程，则不需要post
                            // asio::post(
                            //     session_ptr->GetIoExecutor(),
                            //     [session_ptr]()
                            //     {
                            //         session_ptr->Close();
                            //     });
                        }
                    });
                // TimerEventPtr event_ptr = std::make_shared<TimerEvent>(
                //     session_ptr,
                //     [session_weak = session_ptr->weak_from_this()]()
                //     {
                //         LOG_DEBUG << "OnSession timeout event";
                //         SessionPtr session_ptr = session_weak.lock();
                //         if (session_ptr)
                //         {
                //             LOG_DEBUG << "Session timeout close";
                //             session_ptr->Close(); // 运行在同个线程的timerwheel中
                //             // 如果 TimerWheel 运行与main_ioct中，需要将Close操作post到session_ptr所在线程
                //             // 如果timerwheel使用的是GetLocalTimerWheel，运行在与session_ptr的同个线程，则不需要post
                //             // asio::post(
                //             //     session_ptr->GetIoExecutor(),
                //             //     [session_ptr]()
                //             //     {
                //             //         session_ptr->Close();
                //             //     });
                //         }
                //     });
                // // timer_wheel_->AddTimerEvent(event_ptr);
                // asio::post(
                //     session_ptr->GetIoExecutor(),
                //     [event_ptr]()
                //     {
                //         GetLocalTimerWheel()->AddTimerEvent(event_ptr);
                //     });
                // TimerEventWeak event_weak(event_ptr);
                // session_ptr->SetContext(TimerEventWeak(event_ptr));
                // std::lock_guard<std::mutex> lock(session_mutex_); // 将 erase和emplace都放在主线程，干掉锁
                // std::stringstream ss;
                // ss << std::this_thread::get_id();
                // LOG_DEBUG << "Accept session: " << ss.str(); // debug: 判断emplace和erase都在主线程
                std::size_t session_id = session_ptr->GetId();
                session_map_.emplace(std::make_pair(session_id, session_ptr));
                LOG_DEBUG << "Accept new session: " << std::to_string(session_id);
                session_ptr->SetRequestCallback(
                    [&](const SessionPtr &session, const Request *request, const std::error_code &ec)
                    {
                        LOG_THREAD_ID("SetRequestCallback");
                        this->OnSessionRead(session, request, ec);
                    });
                session_ptr->SetResponseCallback(
                    [&](const SessionPtr &session, std::size_t bytes_transferred, const std::error_code &ec)
                    {
                        this->OnSessionWrite(session, bytes_transferred, ec);
                    });
                session_ptr->SetCloseCallback(
                    [&](const SessionPtr &session, const std::error_code &ec)
                    {
                        // LOG_DEBUG << "Close callback: " << std::to_string(session->GetId());
                        this->OnSessionClose(session, ec);
                    });
                session_ptr->Start(); // 开始接收
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
        // timer_wheel_->Start();
        tcp_server_->Start();
    }

    void PbServer::Stop()
    {
        tcp_server_->Stop();
    }

    void PbServer::OnSessionClose(const SessionPtr &session_ptr, const std::error_code &ec)
    {
        std::size_t id = session_ptr->GetId();
        LOG_THREAD_ID("Session[" + std::to_string(id) + "] close");
        // post到主线程erase，去掉锁
        asio::post(
            tcp_server_->GetIOContext(),
            [this, id]()
            {
                // std::stringstream ss;
                // ss << std::this_thread::get_id();
                // LOG_DEBUG <<"erase thread: "<< ss.str(); // debug: 判断emplace和erase是否否在主线程
                if (session_map_.find(id) != session_map_.end())
                {
                    session_map_.erase(id);
                }
            });
    }

    void PbServer::OnSessionRead(const SessionPtr &session, const Request *request, const std::error_code &ec)
    {
        if (ec)
        {
            session->Close();
        }
        else
        {
            if (!request)
            {
                std::string err_msg = std::to_string(session->GetId()) + " send invalid request.";
                LOG_DEBUG << err_msg;
                session->Close(); // or send msg_id = -1 and error code ??
                return;
            }
            Response *response = dispatcher_->Dispatch(request);
            // if (response->GetMsgId() == "HEARTBEAT")
            // {
            //     // TODO：refresh timeout
            // }
            TimerEventWeak event_weak = std::any_cast<TimerEventWeak>(session->GetContext());
            TimerEventPtr event_ptr = event_weak.lock();
            if (event_ptr)
            {
                event_ptr->RefreshTimeout(30);
                GetLocalTimerManager()->AddTimerEvent(event_ptr);
            }
            session->WriteResponse(response);
            session->Start();
            delete response;
        }
    }

    void PbServer::OnSessionWrite(const SessionPtr &session, std::size_t bytes_transferred, const std::error_code &ec)
    {
        assert(session);
        if (ec)
        {
            session->Close();
        }
    }
}
