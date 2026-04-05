#include "timer_wheel.h"

namespace jl
{

	thread_local std::shared_ptr<TimerWheel> timer_wheel{nullptr};

	std::shared_ptr<TimerWheel> GetTimerWheel()
	{
		if (!timer_wheel)
		{
			timer_wheel = std::make_shared<TimerWheel>();
		}
		return timer_wheel;
	}
}
