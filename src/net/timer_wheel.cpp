#include "timer_wheel.h"
#include <utils/easy_log.hpp>

namespace jl
{

	thread_local std::shared_ptr<TimerWheel> timer_wheel{nullptr};

	uint64_t GetCurrTime()
	{
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	std::shared_ptr<TimerWheel> GetTimerWheel(asio::io_context& ioct)
	{
		if (!timer_wheel)
		{
			timer_wheel = std::make_shared<TimerWheel>(ioct);
		}
		return timer_wheel;
	}

	TimerWheel::TimerWheel(asio::io_context &ioct)
		: ticks_(0),
		  timer_(ioct)
	{
		LOG_DEBUG << "TimerWheel: " << std::to_string(std::intptr_t(this));
		wheel_idx_.push_back(0);
		wheel_idx_.push_back(0);
		wheel_idx_.push_back(0);
		wheel_size_.push_back(60);
		wheel_size_.push_back(60);
		wheel_size_.push_back(24);
		wheels_.emplace_back(Wheel(wheel_size_[0]));
		wheels_.emplace_back(Wheel(wheel_size_[1]));
		wheels_.emplace_back(Wheel(wheel_size_[2]));
	}

	void TimerWheel::AddSessionSafe(const SessionPtr &session)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		AddSession(session);
	}

	void TimerWheel::AddSession(const SessionPtr &session)
	{
		if (session->GetTimeout() > kMaxTimeoutSec)
		{
			session->SetTimeout(kMaxTimeoutSec);
		}
		uint64_t timeout = session->GetTimeout() + wheel_idx_[0];
		int wheel_idx = 0, slot_idx = timeout;
		for (int i = 0; i < wheel_size_.size(); ++i)
		{
			slot_idx = timeout;
			timeout /= wheel_size_[i];
			if (timeout == 0)
			{
				wheel_idx = i;
				break;
			}
		}
		wheels_[wheel_idx][slot_idx].push(session);
	}

	void TimerWheel::Start()
	{
	}

	void TimerWheel::Tick()
	{
		++ticks_;
		// TODO: 遍历当前的 wheel_idx_[0] 中的 slot
		for (int i = 1; i < wheel_size_.size(); ++i)
		{
			if (wheel_idx_[i - 1] == wheel_size_[i - 1])
			{
				wheel_idx_[i - 1] = 0;
				// TODO: 将读取 wheel_idx_[i] 对应的 slot 拿出来放到上一层的 wheel 里面
				++wheel_idx_[i];
			}
		}
		if (wheel_idx_.back() == wheel_size_.back())
			wheel_idx_.back() = 0;
	}

	void TimerWheel::Cancel()
	{
	}

	void TimerWheel::Loop()
	{
	}
    
	TimerWheel::~TimerWheel()
    {
		LOG_DEBUG << "~TimerWheel";
    }
}
