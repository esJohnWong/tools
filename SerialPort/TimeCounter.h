#pragma once
#include <cstdint>

/*
 * 计时器内部使用毫秒
 */
class TimeCounter
{
public:
	TimeCounter();
	~TimeCounter();

	void Start();
	uint64_t End();

private:
	uint64_t frequency_;
	uint64_t start_;
	uint64_t end_;
};

