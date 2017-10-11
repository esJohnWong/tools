#include "Log.h"

#include <time.h>
#include <fstream>

 Log *defaultLog = nullptr;

struct Log::LogInfo
{
	LogConfig conf_;
	std::mutex mutex_;
	LogInfo(LogConfig conf)
		: conf_(std::move(conf))
	{}
};

Log::Log(const LogConfig &conf)
	: data_(new LogInfo(conf))
{
}

Log::~Log()
{
	delete data_;
	data_ = nullptr;
}

void Log::InfoLog(const std::string &log)
{
	if (!(data_->conf_.logLevel & LOG_LEVEL::INFO))
	{
		return;
	}

	if (data_->conf_.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(data_->mutex_);
		WriteLog(log);
	}
}

void Log::WarnLog(const std::string &log)
{
	if (!(data_->conf_.logLevel & LOG_LEVEL::WARN))
	{
		return;
	}

	if (data_->conf_.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(data_->mutex_);
		WriteLog(log);
	}
	return ;
}

void Log::ErroLog(const std::string &log)
{
	if (!(data_->conf_.logLevel & LOG_LEVEL::ERRO))
	{
		return ;
	}

	if (data_->conf_.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(data_->mutex_);
		WriteLog(log);
	}
}


void Log::DebuLog(const std::string &log)
{
	if (!(data_->conf_.logLevel & LOG_LEVEL::DEBU))
	{
		return;
	}

	if (data_->conf_.lockFree)
	{
		WriteLog(log);
	}
	else
	{
		std::lock_guard<std::mutex> lock(data_->mutex_);
		WriteLog(log);
	}
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
	localtime_s(&lctm, &curtime);
	
	std::string timefmt;
	switch (data_->conf_.nameMode)
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
	return data_->conf_.logPath + buf + "." + data_->conf_.logSuffix;
}

std::string Log::GetLogTime() const
{
	time_t curtime = time(nullptr);
	struct tm lctm;
	localtime_s(&lctm, &curtime);

	char buf[50] = { 0 };
	strftime(buf, sizeof(buf), "%F %T", &lctm);
	return buf;
}

void  InitLog(LogConfig conf)
{
	defaultLog = new Log(conf);
}

void  DestroyLog()
{
	delete defaultLog;
	defaultLog = nullptr;
}
