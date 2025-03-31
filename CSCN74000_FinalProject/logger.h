#pragma once
#include <fstream>
#include <ctime>

#define MAX_LOG_PATH_LEN 1024
#define DEFAULT_LOG_PATH "log.txt"
#define MAX_TIMESTAMP_LEN 40
#define LOGGER_FILEOPENEXCEPTION -100

namespace debug
{
	class Logger {

#ifdef TESTING
	public:
#else
	private:
#endif
		std::ofstream msgOutFileStream;

	public:
		Logger(const char* path = DEFAULT_LOG_PATH)
		{
			this->msgOutFileStream.open(path, std::fstream::out | std::ios::app);

		}
		bool isOpen()
		{
			return this->msgOutFileStream.is_open();
		}

		int write(const char* msg, int length)
		{
			std::tm tm{};
			const std::time_t time = std::time(nullptr);
			char* pTime = nullptr;

			pTime = std::asctime(std::localtime(&time));

			if (isOpen())
			{
				if (pTime != nullptr)
				{
					if (!msgOutFileStream.write(pTime, strnlen(pTime, MAX_TIMESTAMP_LEN)))
					{
						throw LOGGER_FILEOPENEXCEPTION;
					}
				}

				if (!msgOutFileStream.write(msg, length))
				{
					throw LOGGER_FILEOPENEXCEPTION;
				}

				msgOutFileStream << std::endl;
			}

			return length;
		}
		~Logger()
		{
			this->msgOutFileStream.close();
		}
	};
}
