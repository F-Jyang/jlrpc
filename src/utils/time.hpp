#pragma once
#include <chrono>

namespace jl
{
	inline uint64_t  GetCurrTimeSec()
	{
		return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}
}