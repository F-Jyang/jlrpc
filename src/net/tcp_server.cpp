#include "tcp_server.h"
#include <utils/easy_log.hpp>

namespace jl
{

    Server::Server(asio::io_context &main_ioct, const std::string &ip, unsigned short port, std::size_t ioct_pool_size)
        : is_stop_(true),
          main_ioct_(main_ioct),
          //   ioct_pool_(ioct_pool_size),
          acceptor_(std::make_shared<Acceptor>(main_ioct, ip, port)),
          signals_(main_ioct)
    {
        // // 1. 解除信号阻塞（防止继承的阻塞掩码）
        // sigset_t sigset;
        // sigemptyset(&sigset);
        // sigaddset(&sigset, SIGINT);
        // sigaddset(&sigset, SIGTERM);
        // pthread_sigmask(SIG_UNBLOCK, &sigset, nullptr);
        // // 2. 注册信号（确保 io_context 未停止）
        // if (main_ioct_.stopped())
        // {
        //     main_ioct_.restart();
        // }
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
#ifdef SIGQUIT
        signals_.add(SIGQUIT);
#endif
        DoAwaitStop();
    }

    void Server::Start()
    {
        is_stop_ = false;
        acceptor_->DoAccept();
        IoContextPool::Instance().Run();
        main_ioct_.run();
        LOG_DEBUG << "Server Stop";

    }

    void jl::Server::Stop()
    {
        if (!is_stop_)
        {
            main_ioct_.stop();
            IoContextPool::Instance().Stop();
        }
        is_stop_ = true;
    }

    void Server::SetConnEstablishCallback(const ConnEstablishCallback &callback)
    {
        acceptor_->SetConnEstablishCallback(callback);
    }

    Server::~Server()
    {
        Stop();
    }

    void Server::DoAwaitStop()
    {
        LOG_DEBUG << "Server wait signal";
        signals_.async_wait(
            [=](std::error_code ec, int sig)
            {
                if (ec)
                    return;
                this->Stop();
                // WaitSignal();  // 如果想继续捕获，再发起一次等待
            });
    }

}
