#include "SerialPort.h"
#include <Windows.h>
#include <assert.h>
#include <thread>
#include <vector>
#include <string>
#include <iostream>

#include "log.h"
#include "TimeCounter.h"

namespace {
	constexpr int COMNAME_LEN = sizeof("COM");

	class Event
	{
	public:
		Event() = default;
		~Event()
		{
			CloseHandle(handle_);
			handle_ = nullptr;
		}

		bool Create(const std::string &name)
		{
			CloseHandle(handle_);
			handle_ = CreateEvent(nullptr, FALSE, FALSE, name.c_str());
			if (handle_ == nullptr)
			{
				return false;
			}
			return true;
		}
		
		bool Set(bool signaled)
		{
			if (signaled)
			{
				return SetEvent(handle_);
			}

			return ResetEvent(handle_);
		}

		HANDLE handle_ = nullptr;
	};

	struct InternalOverlapped : public OVERLAPPED
	{
		InternalOverlapped()
		{
			Internal = 0;
			InternalHigh = 0;
			Offset = 0;
			OffsetHigh = 0;
			hEvent = nullptr;
		}

		bool Create(const std::string &name)
		{
			CloseHandle(hEvent);
			hEvent = CreateEvent(nullptr, TRUE, FALSE, name.c_str());
			if (hEvent == nullptr)
			{
				return false;
			}
			return true;
		}

		void NotSignaled()
		{
			ResetEvent(hEvent);
		}

		~InternalOverlapped()
		{
			CloseHandle(hEvent);
			hEvent = nullptr;
			Internal = 0;
			InternalHigh = 0;
			Offset = 0;
			OffsetHigh = 0;
		}
	};

}

struct SerialPort::SerialInfo
{
	SerialPortConfig conf_;
	HANDLE comHandle_ = INVALID_HANDLE_VALUE;
	std::function<void(const std::vector<uint8_t>&, void*)> callBack_;
	std::thread callBackThd_;
	InternalOverlapped overlaped_;
	Event ctrlEvent_;
	bool enableCallback_ = false;

	SerialInfo(SerialPortConfig info)
		: conf_(info)
	{}
};

SerialPort::SerialPort(SerialPortConfig conf)
	: data_(new SerialInfo(conf))
{
	InitLog();
}


SerialPort::~SerialPort()
{
	delete data_;
	data_ = nullptr;
	DestroyLog();
}

bool SerialPort::Open(bool enableCallback /*= false*/)
{
	data_->enableCallback_ = enableCallback;
	assert(data_->comHandle_ == INVALID_HANDLE_VALUE);
	std::string portName = data_->conf_.portName;
	
	// 如果COM号大于9则使用\\.\COMxxx的形式
	if (data_->conf_.portName.size() > COMNAME_LEN)
	{
		portName = R"(\\.\)" + data_->conf_.portName;
	}

	DWORD attr = 0;
	if (data_->enableCallback_)
	{
		attr = FILE_FLAG_OVERLAPPED;
	}

	// 获得当前串口句柄
	data_->comHandle_  = CreateFile(portName.c_str(), GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_EXISTING, attr, NULL);

	if (data_->comHandle_ == INVALID_HANDLE_VALUE)
	{
		ERROLOG("打开串口CreateFile" + portName+ "失败,GetLastError=" + GetLastError());
		return false;
	}

	auto closeCom = [&handle = data_->comHandle_]
	{
		CloseHandle(handle);
		handle = NULL;
	};

	//设置读超时
	COMMTIMEOUTS timeouts;
	GetCommTimeouts(data_->comHandle_, &timeouts);
	timeouts.ReadIntervalTimeout = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = data_->conf_.readTimeOut;
	timeouts.WriteTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = data_->conf_.writeTimeOut;
	SetCommTimeouts(data_->comHandle_, &timeouts);

	// 设置串口配置信息
	DCB dcb;
	if (!GetCommState(data_->comHandle_, &dcb))
	{
		ERROLOG("获取串口信息GetCommState失败 GetLastError=" + GetLastError());
		closeCom();
		return false;
	}

	dcb.BaudRate = data_->conf_.baudRate; // 数据传输速率
	dcb.ByteSize = data_->conf_.byteSize; // 每字节位数
	dcb.Parity = data_->conf_.parity;     // 校验方式
	dcb.StopBits = data_->conf_.stopBits;
	if (dcb.Parity != NOPARITY)
	{
		dcb.fParity = TRUE;
	}

	if (!SetCommState(data_->comHandle_, &dcb))
	{
		ERROLOG("设置串口信息SetCommState失败,GetLastError=" + GetLastError() + ",portName=" + portName + ",波特率=" + data_->conf_.baudRate +
			",数据位=" + data_->conf_.byteSize + ",校验位=" + data_->conf_.parity + ",停止位=" + data_->conf_.stopBits);
		closeCom();
		return false;
	}

	// 清空缓存
	PurgeComm(data_->comHandle_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	if (data_->enableCallback_)
	{
		auto name = R"(Global\)" + data_->conf_.portName + ".ctrl";
		if (!data_->ctrlEvent_.Create(name))
		{
			ERROLOG("创建控制事件对象失败, GetLastError=" + GetLastError() + ",name=" + name);
			CloseHandle(data_->comHandle_);
			data_->comHandle_ = INVALID_HANDLE_VALUE;
			return false;
		}

		name = R"(Global\)" + data_->conf_.portName + ".serial";
		if (!data_->overlaped_.Create(name))
		{
			ERROLOG("打开事件对象" + name + ",失败, GetLastError=" + GetLastError());
			return false;
		}
	}
	
	return true;
}

bool SerialPort::Close()
{
	assert(data_->comHandle_ != INVALID_HANDLE_VALUE);
	UnregisterCallBack();
	PurgeComm(data_->comHandle_, PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_TXCLEAR);
	auto ret = CloseHandle(data_->comHandle_);
	data_->comHandle_ = INVALID_HANDLE_VALUE;
	return ret;
}

bool SerialPort::RegisterCallBack(std::function<void(const std::vector<uint8_t>&, void *)> callBack, void *param)
{
	if (!data_->enableCallback_)
	{
		ERROLOG("尚未开启回调函数");
		return false;
	}

	if (data_->callBack_)
	{
		ERROLOG("已经有一个回调函数存在");
		return false;
	}

	if (data_->comHandle_ == INVALID_HANDLE_VALUE)
	{
		ERROLOG("无效的串口句柄");
		return false;
	}

	if (!SetCommMask(data_->comHandle_, EV_RXCHAR))
	{
		ERROLOG("设置串口事件监听SetCommMask出错, GetLastError=" + GetLastError());
		return false;
	}

	data_->callBack_ = callBack;
	
	auto readThd = [&]
	{
		DWORD mask = 0, dwErrorFlags = 0, realLen = 0;
		COMSTAT comStat;
		std::vector<uint8_t> buffer;
		HANDLE handles[] = { data_->overlaped_.hEvent, data_->ctrlEvent_.handle_ };

		while (true)
		{
			data_->overlaped_.NotSignaled();
			WaitCommEvent(data_->comHandle_, &mask, &data_->overlaped_);
			if (GetLastError() != ERROR_IO_PENDING)
			{
				ERROLOG("提交异步请求WaitCommEvent错误, GetLastError=" + GetLastError());
				return;
			}

			auto ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
			auto eventno = ret - WAIT_OBJECT_0;
			if (eventno == 1)
			{
				return;
			}
			else if (eventno != 0)
			{
				WARNLOG("WaitForMultipleObjects ret = " + ret);
				continue;
			}

			if (!(mask & EV_RXCHAR))
			{
				continue;
			}

			if (!ClearCommError(data_->comHandle_, &dwErrorFlags, &comStat))
			{
				ERROLOG("清空串口" + data_->conf_.portName + "错误ClearCommError失败,GetLastError=" + GetLastError());
				continue;
			}


			if (comStat.cbInQue == 0)
			{
				continue;
			}

			buffer.resize(comStat.cbInQue);
			realLen = 0;

			if (!ReadFile(data_->comHandle_, buffer.data(), comStat.cbInQue, &realLen, &data_->overlaped_))
			{
				ERROLOG("读取串口" + data_->conf_.portName + "数据ReadFile失败,GetLastError=" + GetLastError());
				continue;
			}

			data_->callBack_(buffer, param);
		}
	};

	data_->callBackThd_ = std::thread(readThd);
	return true;
}

bool SerialPort::UnregisterCallBack()
{
	if (data_->comHandle_ == INVALID_HANDLE_VALUE)
	{
		ERROLOG("串口句柄为空");
		return false;
	}

	if (!data_->ctrlEvent_.Set(true))
	{
		ERROLOG("设置控制事件对象为有信号状态失败, GetLastError=" + GetLastError());
		return false;
	}

	if (data_->callBackThd_.joinable())
	{
		data_->callBackThd_.join();
	}

	data_->callBack_ = std::function<void(const std::vector<uint8_t> &, void*)>();
	return true;
}

std::string SerialPort::Read(uint32_t dataLen /*= 12*/)
{
	assert(data_->comHandle_ != INVALID_HANDLE_VALUE);
	COMSTAT comStat;
	DWORD dwErrorFlags;
	// 清空缓存
	if (!PurgeComm(data_->comHandle_, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT))
	{
		ERROLOG("清空串口" + data_->conf_.portName + "缓存PurgeComm失败,GetLastError=" + GetLastError());
		return "";
	}
	// 阻塞等待输入缓冲区内有数据输入
	// 如果初始化事设置超时失败则用此处超时判断
	TimeCounter timer;
	timer.Start();
	while (true)
	{
		if (!ClearCommError(data_->comHandle_, &dwErrorFlags, &comStat))
		{
			ERROLOG("清空串口" + data_->conf_.portName + "错误ClearCommError失败,GetLastError=" + GetLastError());
			return "";
		}

		if (comStat.cbInQue >= dataLen)
		{
			break;
		}

		if (timer.End() > data_->conf_.readTimeOut)
		{
			ERROLOG("读串口" + data_->conf_.portName + "数据超时,等待时间为" + timer.End() + ",设置超时时间为" + data_->conf_.readTimeOut);
			return "";
		}

		std::this_thread::yield();
	}

	auto bufferLen = dataLen == 0 ? 1024 : dataLen;
	std::vector<char> buffer(bufferLen);
	DWORD realLen = 0;
	
	if (!ReadFile(data_->comHandle_, buffer.data(), bufferLen, &realLen, nullptr))
	{
		ERROLOG("读取串口" + data_->conf_.portName + "数据ReadFile失败,GetLastError=" + GetLastError());
		return "";
	}

	// 读到的数据不能比设置的小
	if (realLen < dataLen)
	{
		ERROLOG("读取串口" + data_->conf_.portName + "数据长度" + realLen + "与期待长度" + dataLen + "不相符");
		return "";
	}

	return std::string(buffer.data(), realLen);
}

bool SerialPort::Write(const std::string &contents)
{
	assert(data_->comHandle_ != INVALID_HANDLE_VALUE);
	COMSTAT ComStat;
	DWORD dwErrorFlags = 0;
	if (!ClearCommError(data_->comHandle_, &dwErrorFlags, &ComStat))
	{
		ERROLOG("清空串口" + data_->conf_.portName + "错误ClearCommError失败,GetLastError=" + GetLastError());
		return false;
	}

	DWORD length = 0;
	if (!WriteFile(data_->comHandle_, contents.data(), contents.length(), &length, nullptr))
	{
		ERROLOG("写串口" + data_->conf_.portName + "数据WriteFile失败,GetLastError=" + GetLastError() + ",数据长度为" + contents.length());
		return false;
	}

	// 判断想要写入的大小与实际写入的大小是否相同
	if (length != contents.length())
	{
		ERROLOG("写串口" + data_->conf_.portName + "数据长度" + length + "与期望长度" + contents.length() + "不相符");
		return false;
	}

	return true;
}

