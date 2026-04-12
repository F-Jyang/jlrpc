#include "timer_wheel.h"
#include <utils/easy_log.hpp>

namespace jl
{

	thread_local std::shared_ptr<TimerWheel> timer_wheel{nullptr};

	std::shared_ptr<TimerWheel> GetTimerWheel(asio::io_context &ioct)
	{
		if (!timer_wheel)
		{
			timer_wheel = std::make_shared<TimerWheel>(ioct);
		}
		return timer_wheel;
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
		wheels_[wheel_idx][slot_idx].push(event_ptr);
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
			std::lock_guard<std::mutex> lock(mutex_);
			++wheel_idx_[0]; // 前进一个slot
			del_slots[0].swap(wheels_[0][wheel_idx_[0]  % wheels_[0].size()]);
			for (int i = 1; i < wheels_size && wheel_idx_[i - 1] == wheels_[i - 1].size(); ++i)
			{
				wheel_idx_[i - 1] = 0; // 上一个时间轮回到第一个slot
				++wheel_idx_[i]; // 拿出当前时间轮的slot，需要注意索引不能越界
				del_slots[i].swap(wheels_[i][wheel_idx_[i] % wheels_[i].size()]);
				// 将当前时间轮即将超时的TimerEvent下放到上一个时间轮中
				while (!del_slots[i].empty())
				{
					AddTimerEventUnSafe(del_slots[i].front());
					del_slots[i].pop();
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
}
