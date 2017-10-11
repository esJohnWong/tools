#include <iostream>
#include <thread>
#include "SerialPort.h"


int main()
{
	SerialPortConfig conf;
	std::cout << "COM: ";
	std::cin >> conf.portName;
	std::cout << "buadRate: ";
	std::cin >> conf.baudRate;

	SerialPort com(conf);

	if (!com.Open(true))
	{
		std::cout << "open failed" << std::endl;
	}
	else
	{
		std::cout << "open success" << std::endl;
	}

	int cnt = 0;

	auto call = [&cnt](const std::vector<uint8_t> &buffer, void *)
	{
		std::string str(reinterpret_cast<const char *>(buffer.data()), buffer.size());
		std::cout << str << std::endl;
		++cnt;
	};

	if (!com.RegisterCallBack(call, nullptr))
	{
		std::cout << "×¢²áÊ§°Ü" << std::endl;
	}
	else
	{
		std::cout << "×¢²á³É¹¦" << std::endl;
	}

	
	while (cnt < 2)
	{
		std::this_thread::yield();
	}

	if (!com.Close())
	{
		std::cout << "Close failed" << std::endl;
	}
	else
	{
		std::cout << "Close success" << std::endl;
	}

	return 0;
}