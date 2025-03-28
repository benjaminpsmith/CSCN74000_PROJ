#pragma once
#include <vector>
#include <string>
#include <iostream>

#define LOGMSG_LIMIT 20
class Log
{
	std::vector<std::string> log;

public:

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
	Log messages;

public:

	Menu()
	{
	}

	const Menu& operator<<(const char* logMessage)
	{
		messages << logMessage;
		return *this;
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