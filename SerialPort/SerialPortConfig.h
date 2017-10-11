#pragma once
#include <cstdint>
#include <string>

// 串口号配置
struct SerialPortConfig
{
	std::string portName = "COM1";		// 端口号名称
	uint32_t baudRate = 1200;			// 波特率
	uint8_t byteSize = 8;				// 数据位
	uint8_t parity = 0;					// 奇偶校验位
	uint8_t stopBits = 0;				// 停止位
	uint32_t readTimeOut = 2000;		// 读超时时间 毫秒 0代表未设置
	uint32_t writeTimeOut = 2000;		// 写超时时间 毫秒 0代表未设置
};