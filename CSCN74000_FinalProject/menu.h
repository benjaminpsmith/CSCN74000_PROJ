#pragma once
#include <vector>
#include <string>
#include <iostream>
#include "logger.h"

#define LOGMSG_LIMIT 20
#define LOG_ENTRY_MAX_LEN 512

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

	const Log& operator<<(PacketDef& packet)
	{
		int converted = 0;
		char* packetBuff = new char[LOG_ENTRY_MAX_LEN];
		char* hexBuff = new char[LOG_ENTRY_MAX_LEN];
		memset(packetBuff, 0, LOG_ENTRY_MAX_LEN);
		memset(hexBuff, 0, LOG_ENTRY_MAX_LEN);
		packet.Serialize(packetBuff);
		converted = sprintf(hexBuff, "%x", packetBuff);
		
		writeToFile(hexBuff, converted);
		return *this;
	}

	const Log& operator<<(const char* logMessage)
	{
		if (log.size() > LOGMSG_LIMIT)
		{
			//erase all but the last 10 entries
			log.erase(log.begin(), log.end() - LOGMSG_LIMIT);
		}
		
		std::string message = logMessage;
		log.push_back(message);


		return *this;
	}
	const Log& operator<<(int& number)
	{
		if (log.size() > LOGMSG_LIMIT)
		{
			//erase all but the last 10 entries
			log.erase(log.begin(), log.end() - LOGMSG_LIMIT);
		}

		std::string message = " ";
		message += std::to_string(number);
		log.at(log.size() - 1).append(message);

		return *this;
	}
	
	bool writeToFile(char* logMessage, int length)
	{

		if (fileLogger.isOpen())
		{
			fileLogger.write(logMessage, length);
			return true;
		}

		return false;
	}

	void print()
	{
		for(int i = 0; i < log.size(); i++)
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

	Menu(const char* path = DEFAULT_LOG_PATH): messages(path)
	{

	}

	const Menu& operator<<(PacketDef& packet)
	{
		messages << packet;
		return *this;
	}

	const Menu& operator<<(const char* logMessage)
	{
		messages << logMessage;
		return *this;
	}

	bool writeToFileLog(char* msg, int length)
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
		std::system("cls");
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