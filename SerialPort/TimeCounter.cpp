#include "TimeCounter.h"

#include <cstdint>
#include <ctime>
#include <Windows.h>


TimeCounter::TimeCounter()
	: frequency_(1)
	, start_(0)
	, end_(0)
{
	LARGE_INTEGER frequency;    // 每秒钟的频率 
	if (QueryPerformanceFrequency(&frequency))
	{
		frequency_ = frequency.QuadPart;
	}
}


TimeCounter::~TimeCounter()
{

}

void TimeCounter::Start()
{
	LARGE_INTEGER timeStart;    //开始时间  
	if (QueryPerformanceCounter(&timeStart))
	{
		start_ = timeStart.QuadPart;
	}
	else
	{
		start_ = time(nullptr);
	}
}

uint64_t TimeCounter::End()
{
	LARGE_INTEGER timeEnd;      //结束时间  
	if (QueryPerformanceCounter(&timeEnd))
	{
		end_ = timeEnd.QuadPart;
	}
	else
	{
		end_ = time(nullptr);
	}

	// frequency_是每秒钟的次数，end_ - start_得到总次数 除以frequency_ 得到秒数，乘以1000到毫秒
	auto elapsed = (end_ - start_) * 1000 / frequency_;
	return elapsed;
}
