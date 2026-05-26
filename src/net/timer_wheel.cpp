#include "timer_wheel.h"
#include <utils/easy_log.hpp>
#include <thread>

namespace jl
{
	thread_local std::shared_ptr<SessionTimerManager> kTimerManager{nullptr};

	bool InitLocalTimerManager(asio::io_context &ioct)
	{
		if (!kTimerManager)
		{
			kTimerManager = std::make_shared<SessionTimerManager>(ioct);
		}
		return kTimerManager != nullptr;
	}

	// void LocalAddTimerEvent(const SessionPtr &session_ptr, const TimerEventCallback &callback)
	// {
	// 	TimerEventPtr event_ptr = std::make_shared<TimerEvent>(
	// 		session_ptr,
	// 		[session_weak = session_ptr->weak_from_this()]()
	// 		{
	// 			LOG_DEBUG << "OnSession timeout event";
	// 			SessionPtr session_ptr = session_weak.lock();
	// 			if (session_ptr)
	// 			{
	// 				LOG_DEBUG << "Session timeout close";
	// 				session_ptr->Close();
	// 				// 运行在同个线程的timerwheel中
	// 				// 如果 TimerWheel 运行与main_ioct中，需要将Close操作post到session_ptr所在线程
	// 				// 如果timerwheel使用的是GetLocalTimerWheel，运行在与session_ptr的同个线程，则不需要post
	// 				// asio::post(
	// 				//     session_ptr->GetIoExecutor(),
	// 				//     [session_ptr]()
	// 				//     {
	// 				//         session_ptr->Close();
	// 				//     });
	// 			}
	// 		});
	// 	// timer_wheel_->AddTimerEvent(event_ptr);
	// 	// session_ptr->GetIoExecutor(),
	// 	GetLocalTimerWheel()->AddTimerEvent(event_ptr);
	// 	TimerEventWeak event_weak(event_ptr);
	// 	session_ptr->SetContext(TimerEventWeak(event_ptr));
	// }

	std::shared_ptr<SessionTimerManager> GetLocalTimerManager()
	{
		assert(kTimerManager);
		return kTimerManager;
	}

	TimerWheel::TimerWheel(asio::io_context &ioct)
		: timer_(ioct),
		  interval_(1)
	{
		LOG_DEBUG << "TimerWheel: " << std::to_string(std::intptr_t(this));
		wheel_idx_.push_back(0);
		wheel_idx_.push_back(0);
		wheel_idx_.push_back(0);
		wheels_.emplace_back(Wheel(60));
		wheels_.emplace_back(Wheel(60));
		wheels_.emplace_back(Wheel(24));
		interval_ = 1;
	}

	void TimerWheel::AddTimerEvent(const TimerEventPtr &event_ptr)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		AddTimerEventUnSafe(event_ptr);
	}

	void TimerWheel::AddTimerEventUnSafe(const TimerEventPtr &event_ptr)
	{
		uint64_t curr_time = GetCurrTimeSec();
		int interval = event_ptr->arrive_time - curr_time;
		if (interval <= 0) // 立即过期
			return;
		// LOG_DEBUG << "Interval: "  << std::to_string(interval);
		int wheel_idx = 0, slot_idx = interval + wheel_idx_[0];
		// TODO: 修改AddTimerEvent的边界条件
		for (int i = 0; i < wheels_.size(); ++i)
		{
			slot_idx = interval + wheel_idx_[i];
			interval = (interval + wheel_idx_[i]) / wheels_[i].size();
			if (interval == 0)
			{
				wheel_idx = i;
				break;
			}
		}
		wheels_[wheel_idx][slot_idx].insert(event_ptr);
	}

	void TimerWheel::ResetTimerEvent(const TimerEventPtr &event_ptr)
	{
		AddTimerEvent(event_ptr);
	}

	void TimerWheel::Start()
	{
		timer_.expires_after(std::chrono::seconds(interval_));
		timer_.async_wait(
			[&](const std::error_code &ec)
			{
				if (ec && ec != asio::error::operation_aborted)
				{
					LOG_DEBUG << ec.message();
				}
				Tick();
				Start();
			});
	}

	void TimerWheel::Tick()
	{
		uint64_t now = GetCurrTimeSec();
		std::size_t wheels_size = wheels_.size();
		std::vector<Slot> del_slots(wheels_size);
		{
			std::lock_guard<std::mutex> lock(mutex_); // 如果将timerwheel设置为线程局部变量，则可以不加锁
			++wheel_idx_[0];						  // 前进一个slot
			del_slots[0].swap(wheels_[0][wheel_idx_[0] % wheels_[0].size()]);
			for (int i = 1; i < wheels_size && wheel_idx_[i - 1] == wheels_[i - 1].size(); ++i)
			{
				wheel_idx_[i - 1] = 0; // 上一个时间轮回到第一个slot
				++wheel_idx_[i];	   // 拿出当前时间轮的slot，需要注意索引不能越界
				del_slots[i].swap(wheels_[i][wheel_idx_[i] % wheels_[i].size()]);
				// 将当前时间轮即将超时的TimerEvent下放到上一个时间轮中
				for (auto it = del_slots[i].begin(); it != del_slots[i].end(); ++it)
				{
					AddTimerEventUnSafe(*it);
				}
			}
			if (wheel_idx_.back() == wheels_.back().size())
				wheel_idx_.back() = 0;
			// ++ticks_;
		}
		// del_slots中的元素析构，执行callback，避免加锁。
	}

	void TimerWheel::Cancel()
	{
		timer_.cancel();
	}

	TimerWheel::~TimerWheel()
	{
		LOG_DEBUG << "~TimerWheel";
	}

	SessionTimerManager::SessionTimerManager(asio::io_context &ioct)
		: timer_wheel_(ioct)
	{
		// 初始化并启动本线程的timer_wheel_
		timer_wheel_.Start();
	}

	void SessionTimerManager::PostTimerEvent(const SessionPtr &session_ptr, TimerEventCallback callback)
	{
		// 将 TimerEvent Post 到 session_ptr 所在 io_context
		asio::post(
			session_ptr->GetIoExecutor(),
			[=]()
			{
				LOG_THREAD_ID("PostTimerEvent");
				TimerEventPtr event_ptr = std::make_shared<TimerEvent>(session_ptr, callback);
				GetLocalTimerManager()->timer_wheel_.AddTimerEvent(event_ptr);
				TimerEventWeak event_weak(event_ptr);
				session_ptr->SetContext(TimerEventWeak(event_weak));
			});
	}

	void SessionTimerManager::AddTimerEvent(const TimerEventPtr &timer_event_ptr)
	{
		LOG_THREAD_ID("AddTimerEvent");
		timer_wheel_.AddTimerEvent(timer_event_ptr);
	}
}
