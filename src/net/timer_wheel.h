/// @brief 时间轮
/// @author Jyang.
/// @date 2026-4-3

#pragma once
#include <interface/i_session.h>
#include <asio/steady_timer.hpp>
#include <queue>
#include <memory>
#include <mutex>
#include <functional>

namespace jl
{

    constexpr std::size_t kMaxTimeoutSec = 60 * 60 * 24; // 暂时支持最长一天的超时
    
    class TimerWheel;
    std::shared_ptr<TimerWheel> GetTimerWheel(asio::io_context& ioct);

    uint64_t GetCurrTime();

    struct TimePos
    {

        TimePos(uint64_t s) : sec(s) {}

        bool operator<(const TimePos &pos)
        {
            return sec < pos.sec;
        }

        bool operator>(const TimePos &pos)
        {
            return sec > pos.sec;
        }

        bool operator==(const TimePos &pos)
        {
            return sec == pos.sec;
        }

        TimePos &operator++()
        {
            ++sec;
            return *this;
        }

        int64_t sec;
    };

    struct TimerEvent
    {
        TimerEvent(const SessionPtr &session)
            : session(session),
              timeout_cnt(3),
              timeout_pos(session->GetTimeout())
        {
        }

        TimePos timeout_pos;             // 超时时间
        int32_t timeout_cnt;             // 超时次数
        std::weak_ptr<ISession> session; // session的weak指针
    };

    /// @brief 可以实现一个三层的时间轮，支持s、min、h
    /// @desc 目前的TimerWheel是线程不安全的
    class TimerWheel
    {
        using Slot = std::queue<TimerEvent>;
        using Wheel = std::vector<Slot>;

    public:
        TimerWheel(asio::io_context& ioct);

        void AddSessionSafe(const SessionPtr &session);

        void AddSession(const SessionPtr &session);

        void Start();

        void Tick();

        void Cancel();

        void Loop();

        ~TimerWheel();
    private:
        std::mutex mutex_;
        uint64_t ticks_;
        asio::steady_timer timer_;
        std::vector<std::size_t> wheel_size_; // 每个wheel的size
        std::vector<Wheel> wheels_;           // wheel
        std::vector<int> wheel_idx_;          // 当前轮询的wheel的索引
        std::function<void(const SessionPtr&)> callback_;
    };
}
