#pragma once
#include <asio/io_context.hpp>
#include <thread>

namespace jl
{
    class IoContextPool
    {
        using IoContextPtr = std::shared_ptr<asio::io_context>;
        using IoContextWork = asio::executor_work_guard<asio::io_context::executor_type>;
#define MakeIoContextPtr std::make_shared<asio::io_context>();

    public:
        explicit IoContextPool(/*std::size_t size 后面由config决定*/);

        static IoContextPool &Instance();

        asio::io_context &GetIoContext();

        void Run();

        void Stop();

        IoContextPool(const IoContextPool &) = delete;
        IoContextPool(IoContextPool &&) = delete;
        IoContextPool &operator=(const IoContextPool &) = delete;
        IoContextPool &operator=(IoContextPool &&) = delete;

    private:
        std::unordered_map<std::thread::id,IoContextPtr> ioct_map_;
        std::vector<IoContextPtr> ioct_pool_;
        std::vector<IoContextWork> work_pool_;
        std::vector<std::thread> threads_;
        std::size_t next_ioct_idx_;
    };
}