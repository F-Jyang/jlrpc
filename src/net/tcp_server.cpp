#include "tcp_server.h"
#include <utils/easy_log.hpp>

namespace jl
{

    Server::Server(asio::io_context &main_ioct, const std::string &ip, unsigned short port, std::size_t ioct_pool_size) : is_stop_(true),
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
        // DoAwaitStop();
        // while (thread_cnt > 0)
        //{
        //    io_threads_.emplace_back(std::make_unique<std::thread>(
        //        [=]()
        //        {
        //            ioct_.run();
        //        }));
        //    --thread_cnt;
        //}
        // while (!stop_)
        //{ // 等待停止
        //    std::this_thread::sleep_for(std::chrono::milliseconds(200));
        //}
    }

    void jl::Server::Stop()
    {
        if (!is_stop_)
        {
            main_ioct_.stop();
            IoContextPool::Instance().Stop();
        }
        is_stop_ = true;
        // LOG_WARN("Server stop.");
        // ioct_.stop();
        // for (std::size_t i = 0; i < io_threads_.size(); ++i)
        //{
        //    if (io_threads_[i] && io_threads_[i]->joinable())
        //    {
        //        io_threads_[i]->join();
        //    }
        //}
        // io_threads_.clear();
        // LOG_WARN("Server stop finish.");
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
                // LOG_WARN("Caught signal:{}", sig);
                this->Stop();
                // this->Stop(); // bug: 这里调用Stop，join的时候可能会在非主线程调用，导致崩溃
                // WaitSignal();  // 如果想继续捕获，再发起一次等待
            });
    }

}
