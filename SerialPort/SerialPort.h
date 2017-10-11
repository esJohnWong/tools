#pragma once
#include <string>
#include <vector>
#include <functional>

#include "SerialPortConfig.h"

class SerialPort
{
public:
	SerialPort(SerialPortConfig conf);
	~SerialPort();

	bool Open(bool enableCallback = false);
	bool Close();

	bool RegisterCallBack(std::function<void(const std::vector<uint8_t>&, void *)> callBack, void *param);
	bool UnregisterCallBack();

	std::string Read(uint32_t dataLen = 12);
	bool Write(const std::string &contents);

private:
	struct SerialInfo;
	SerialInfo *data_;
};

