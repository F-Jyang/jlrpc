#include "io_context_pool.h"

namespace jl
{
    IoContextPool::IoContextPool()
        : next_ioct_idx_(0)
    {
        int32_t size = std::thread::hardware_concurrency(); // TODO使用config替代
        if (size <= 0)
        {
            size = 1;
        }
        for (std::size_t i = 0; i < size; ++i)
        {
            IoContextPtr ioct_ptr = MakeIoContextPtr;
            ioct_pool_.emplace_back(ioct_ptr);
            work_pool_.emplace_back(asio::make_work_guard(*ioct_ptr));
        }
    }

    IoContextPool &IoContextPool::Instance()
    {
        static IoContextPool pool;
        return pool;
    }

    asio::io_context &IoContextPool::GetIoContext()
    {
        // 同一时间内应该只有一个线程（Acceptor::OnAccept）调用该函数
        asio::io_context &ioct = *ioct_pool_[next_ioct_idx_];
        next_ioct_idx_ = (next_ioct_idx_ + 1) % ioct_pool_.size();
        return ioct;
    }

    void IoContextPool::Run()
    {
        std::vector<std::thread> threads;
        for (std::size_t i = 0; i < ioct_pool_.size(); ++i)
        {
            threads_.emplace_back(
                [this, i]()
                {
                    ioct_pool_[i]->run();
                });
        }
    }

    void IoContextPool::Stop()
    {
        for (std::size_t i = 0; i < ioct_pool_.size(); ++i)
        {
            ioct_pool_[i]->stop();
        }
        for (std::size_t i = 0; i < ioct_pool_.size(); ++i)
        {
            threads_[i].join();
        }
    }
}
