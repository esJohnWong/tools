#include "Log.h"

#include <time.h>
#include <fstream>

Log::Log(const LogConf &conf)
	: m_conf(conf)
{
}

Log::~Log()
{
}

void Log::InfoLog(const std::string &log)
{
	if (!(m_conf.logLevel & LOG_LEVEL::INFO))
	{
		return;
	}

	if (m_conf.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		WriteLog(log);
	}

	return ;
}

void Log::WarnLog(const std::string &log)
{
	if (!(m_conf.logLevel & LOG_LEVEL::WARN))
	{
		return;
	}

	if (m_conf.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		WriteLog(log);
	}
	return ;
}

void Log::ErroLog(const std::string &log)
{
	if (!(m_conf.logLevel & LOG_LEVEL::ERRO))
	{
		return ;
	}

	if (m_conf.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		WriteLog(log);
	}
	return;
}

void Log::WriteLog(const std::string &log)
{
	std::ofstream outlog(GetLogFilePath(), std::ios::app);
	outlog << "[" << GetLogTime() << " " << log << std::endl;
}

std::string Log::GetLogFilePath() const
{
	time_t curtime = time(nullptr);
	struct tm lctm;
	localtime_r(&curtime, &lctm);
	
	std::string timefmt;
	switch (m_conf.nameMode)
	{
	case NAME_MODE::YYYYMMDD:
	{
		timefmt = "%F";
		break;
	}

	case NAME_MODE::YYYYMMDDHH:
	{
		timefmt = "%F H";
		break;
	}
	default:
		break;
	}

	char buf[50] = { 0 };
	strftime(buf, sizeof(buf), timefmt.c_str(), &lctm);
	return m_conf.logPath + buf + "." + m_conf.logSuffix;
}

std::string Log::GetLogTime() const
{
	time_t curtime = time(nullptr);
	struct tm lctm;
	localtime_r(&curtime, &lctm);

	char buf[50] = { 0 };
	strftime(buf, sizeof(buf), "%F %T", &lctm);
	return buf;
}
