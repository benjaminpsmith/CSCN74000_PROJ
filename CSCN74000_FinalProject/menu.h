#ifndef HPP_MENU
#define HPP_MENU

#include <vector>
#include <string>
#include <iostream>
#include "logger.h"
#include "packet.h"

#define LOGMSG_LIMIT 20
#define LOG_ENTRY_MAX_LEN 512
#define ERROR_NULL_PTR -99
#define MENU_FILEWRITEEXCEPTION -101
#define MENU_SCREENOUTPUTEXCEPTION -102
#define LOG_MAX_BUFFER_LENGTH 512

namespace ui
{
	class Log
	{
#ifdef TESTING
	public:
#else
	private:
#endif
		std::vector<std::string> log;
		debug::Logger fileLogger;

	public:

		Log(const char* path) : fileLogger(path) {};

		const Log& operator<<(PacketData::PacketDef& packet)
		{
			char format[3] = "%x";
			void* setRet = nullptr;
			int converted = 0;
			char packetBuff[LOG_ENTRY_MAX_LEN];
			char hexBuff[LOG_ENTRY_MAX_LEN];
			int bytes = 0;

			setRet = memset(&packetBuff[0], 0, LOG_ENTRY_MAX_LEN);
			if (setRet == nullptr)
			{
				throw ERROR_NULL_PTR;
			}


			setRet = memset(&hexBuff[0], 0, LOG_ENTRY_MAX_LEN);
			if (setRet == nullptr)
			{
				throw ERROR_NULL_PTR;
			}

			bytes = packet.Serialize(&packetBuff[0]);

			converted = sprintf_s(&hexBuff[0], LOG_ENTRY_MAX_LEN, &format[0], &packetBuff[0], bytes);

			if (!writeToFile(&hexBuff[0], converted))
			{
				throw MENU_FILEWRITEEXCEPTION;
			}
			return *this;
		}

		const Log& operator<<(const char* logMessage)
		{
			if (log.size() > LOGMSG_LIMIT)
			{
				//erase all but the last 10 entries
				auto iterator = log.erase(log.begin(), log.end() - LOGMSG_LIMIT);
			}

			std::string message = logMessage;
			log.push_back(message);


			return *this;
		}
		const Log& operator<<(const int& number)
		{
			if (log.size() > LOGMSG_LIMIT)
			{
				//erase all but the last 10 entries
				auto iterator = log.erase(log.begin(), log.end() - LOGMSG_LIMIT);
			}

			std::string message = " ";
			message += std::to_string(number);

			std::basic_string<char, std::char_traits<char>, std::allocator<char>> appendRet = log.at(log.size() - 1).append(message);

			return *this;
		}

		bool writeToFile(const char* logMessage, int length)
		{
			bool ret = false;
			char hexBuff[LOG_MAX_BUFFER_LENGTH];
			char format[3] = "%x";

			if (fileLogger.isOpen())
			{
				int converted = 0; 
				converted = sprintf_s(&hexBuff[0], LOG_MAX_BUFFER_LENGTH, &format[0], logMessage, length);

				if (fileLogger.write(&hexBuff[0], converted) <= 0)
				{
					throw MENU_FILEWRITEEXCEPTION;
				}
				ret = true;
			}

			return ret;
		}

		void print()
		{
			for (int i = 0; i < log.size(); i++)
			{
				std::cout << log.at(i) << std::endl;
			}
		}
	};

	class Menu
	{
#ifdef TESTING
	public:
#else
	private:
#endif

		Log messages;

	public:

		Menu(const char* path = DEFAULT_LOG_PATH) : messages(path)
		{

		}

		const Menu& operator<<(PacketData::PacketDef& packet)
		{
			messages << packet;
			return *this;
		}

		const Menu& operator<<(const char* logMessage)
		{
			messages << logMessage;
			return *this;
		}

		bool writeToFileLog(const char* msg, int length)
		{
			return messages.writeToFile(msg, length);
		}

		Log* getLog()
		{
			return &this->messages;
		}

		void printLog()
		{
			//clear the console
			char clsCmd[4] = "cls";
			int ret = std::system(&clsCmd[0]);
			if (ret != 0)
			{
				messages << "Failed to print the log";
			}
			//print the log
			messages.print();
		}

		void printMenu()
		{
			printLog();

			//print the menu
			std::cout << "1. Shutdown" << std::endl;
		}
	};

}



#endif HPP_MENU