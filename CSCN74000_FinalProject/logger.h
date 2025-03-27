#pragma once
#include <fstream>
#include <ctime>

#define MAX_LOG_PATH_LEN 1024
#define DEFAULT_LOG_PATH "log.txt"

class Logger {
	std::ofstream msgOutFileStream;

public:
	Logger(const char* path = DEFAULT_LOG_PATH)
	{
		this->msgOutFileStream.open(path, std::fstream::out);

	}
	bool isOpen()
	{
		return this->msgOutFileStream.is_open();
	}
	const Logger& write(const char* msg, int length)
	{
		std::tm tm{};
		const std::time_t time = std::time(nullptr);
		char* pTime = std::asctime(std::localtime(&time));
		if (isOpen())
		{
			msgOutFileStream.write(pTime, strlen(pTime));
			msgOutFileStream.write(": ", 2);
			msgOutFileStream.write(msg, length);
			msgOutFileStream << std::endl;
		}
		
		return *this;
	}
	~Logger()
	{
		this->msgOutFileStream.close();
	}
};