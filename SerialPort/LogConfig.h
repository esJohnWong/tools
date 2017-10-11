#pragma once
#include <cstdint>
#include <string>

enum LOG_LEVEL
{
	INFO = 0x01,
	WARN = 0x02,
	ERRO = 0x04,
	DEBU = 0x08,
	ALL = 0x0f
};

enum NAME_MODE
{
	YYYYMMDD,		// 2017-03-12
	YYYYMMDDHH,		// 2017-03-12 19
};

struct LogConfig
{
	std::string logPath = "log";
	std::string logSuffix = "log";
	uint32_t logLevel = LOG_LEVEL::ALL;
	NAME_MODE nameMode = NAME_MODE::YYYYMMDD;
	bool lockFree = false;
};