/// @brief 时间轮
/// @author Jyang.
/// @date 2026-4-3

#pragma once
#include <interface/i_session.h>
#include <asio/steady_timer.hpp>
#include <asio/post.hpp>
#include <utils/time.hpp>
#include <queue>
#include <memory>
#include <mutex>
#include <functional>
#include <set>

namespace jl
{

    constexpr std::size_t kMaxTimeoutSec = 60 * 60 * 24; // 暂时支持最长一天的超时

    class TimerEvent;
    using TimerEventPtr = std::shared_ptr<TimerEvent>;
    using TimerEventWeak = std::weak_ptr<TimerEvent>;
    using TimerEventCallback = std::function<void()>;

    class TimerWheel;
    class SessionTimerManager;

    bool InitLocalTimerManager(asio::io_context &ioct);

    std::shared_ptr<SessionTimerManager> GetLocalTimerManager();

    // void LocalAddTimerEvent(const SessionPtr &session_ptr, const TimerEventCallback &callback);

    /*
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
    */

    struct TimerEvent
    {
        TimerEvent(const SessionPtr &session_ptr, const TimerEventCallback &cb)
            : callback(cb),
              arrive_time(0)
        // session(session_ptr)
        {
            int timeout = kMaxTimeoutSec;
            if (session_ptr->GetTimeout() < kMaxTimeoutSec)
            {
                timeout = session_ptr->GetTimeout();
            }
            arrive_time = GetCurrTimeSec() + timeout;
        }

        void RefreshTimeout(uint64_t timeout)
        {
            arrive_time = GetCurrTimeSec() + timeout;
        }

        ~TimerEvent()
        {
            if (callback)
            {
                callback();
            }
        }

        uint64_t arrive_time; // 超时时间
        // SessionWeak session;
        TimerEventCallback callback;
    };

    /// @brief 可以实现一个三层的时间轮，支持s、min、h
    /// @desc 目前的TimerWheel是线程不安全的
    class TimerWheel
    {

        struct TimerEventCmp
        {
            bool operator()(const TimerEventPtr &p1, const TimerEventPtr &p2) const
            {
                return p1.get() < p2.get();
            }
        };

        using Slot = std::set<TimerEventPtr, TimerEventCmp>;
        using Wheel = std::vector<Slot>;

    public:
        TimerWheel(asio::io_context &ioct);

        // void AddTimerEventSafe(const TimerEventPtr& event_ptr);

        void AddTimerEvent(const TimerEventPtr &event_ptr);

        void AddTimerEventUnSafe(const TimerEventPtr &event_ptr);

        void ResetTimerEvent(const TimerEventPtr &event_ptr);

        void Start();

        void Tick();

        void Cancel();

        ~TimerWheel();

    private:
        std::size_t interval_;
        std::mutex mutex_;
        asio::steady_timer timer_;
        std::vector<Wheel> wheels_;  // wheel
        std::vector<int> wheel_idx_; // 当前轮询的wheel的索引
    };

    class SessionTimerManager
    {
    public:
        SessionTimerManager(asio::io_context &ioct);
        
        /// @brief 向session_ptr所在线程的timerManager添加TimerEvent，同时设置session_ptr的Context为TimerEventWeak
        /// @param session_ptr 
        /// @param callback 
        static void PostTimerEvent(const SessionPtr &session_ptr, TimerEventCallback callback);
    
        /// @brief 向当前线程的timerManager添加TimerEvent
        /// @param timer_event_ptr 
        void AddTimerEvent(const TimerEventPtr& timer_event_ptr);
    private:
        TimerWheel timer_wheel_;
    };
}
