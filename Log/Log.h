#pragma once

#include <string>
#include <mutex>

class LogString
{
	friend LogString operator+(const LogString &lhs, const LogString &rhs)
	{
		return lhs.ToString() + rhs.ToString();
	}
public:

	LogString(const char *str)
		: m_str(str)
	{}

	LogString(const std::string &str)
		: m_str(str)
	{}

	LogString(char var)
		: m_str(1, var)
	{
	}

	LogString(unsigned char var)
		: m_str(std::to_string(var))
	{}

	LogString(int var)
		: m_str(std::to_string(var))
	{}

	LogString(unsigned int var)
		: m_str(std::to_string(var))
	{}

	LogString(long var)
		: m_str(std::to_string(var))
	{}

	LogString(unsigned long var)
		: m_str(std::to_string(var))
	{}

	LogString(long long var)
		: m_str(std::to_string(var))
	{}

	LogString(unsigned long long var)
		: m_str(std::to_string(var))
	{}

	LogString(float var)
	{
		char buf[50] = { 0 };
		snprintf(buf, sizeof(buf), "%.3f", var);
		m_str = buf;
	}

	LogString(double var)
	{
		char buf[50];
		snprintf(buf, sizeof(buf), "%.3lf", var);
		m_str = buf;
	}

	operator std::string() const
	{
		return m_str;
	}

private:
	std::string ToString() const
	{
		return m_str;
	}

	std::string m_str;
};

class Log
{
public:
	static constexpr const char *INFOS = "INFO";
	static constexpr const char *WARNS = "WARN";
	static constexpr const char *ERROS = "ERRO";

	enum LOG_LEVEL
	{
		INFO = 0x01,
		WARN = 0x02,
		ERRO = 0x04,
		ALL  = 0x07
	};

	enum NAME_MODE
	{	
		YYYYMMDD,		// 2017-03-12
		YYYYMMDDHH,		// 2017-03-12 19
	};

	struct LogConf
	{
		std::string logPath = "log";
		std::string logSuffix = "txt";
		LOG_LEVEL logLevel = LOG_LEVEL::ALL;
		NAME_MODE nameMode = NAME_MODE::YYYYMMDD;
		bool lockFree = false;
	};

	Log(const LogConf &conf);
	~Log();

	void InfoLog(const std::string &log);
	void WarnLog(const std::string &log);
	void ErroLog(const std::string &log);

#define Info(log) \
	InfoLog(LogString(Log::INFOS) + "," + __FILE__ + "," + __LINE__ + "," + __FUNCTION__ + "] " + log)

#define Warn(log) \
	WarnLog(LogString(Log::WARNS) + "," + __FILE__ + "," + __LINE__ + "," + __FUNCTION__ + "] " + log)

#define Erro(log) \
	ErroLog(LogString(Log::ERROS) + "," + __FILE__ + "," + __LINE__ + "," + __FUNCTION__ + "] " + log)

private:
	void WriteLog(const std::string &log);
	std::string GetLogFilePath() const;
	std::string GetLogTime() const;

	LogConf m_conf;
	std::mutex m_mutex;
};

