#include "blackbox.h"
BlackBox::BlackBox()
{
}

int BlackBox::loadData(char* pathToData)
{
	int retVal;
	int sequence;
	char buffer[MAX_PACKET_LENGTH];
	char* pBuffer;

	retVal = 0;
	sequence = 0;
	pBuffer = buffer;

	if (this->packetizedBlackBoxData.size())
	{
		this->packetizedBlackBoxData.clear();
	}


	//altitude, position, speed, heading
	try {
		this->blackBoxFileStream.open(pathToData, std::fstream::in);
		while (this->blackBoxFileStream.is_open() && !this->blackBoxFileStream.eof())
		{
			Packet entry(MAX_BODY_LENGTH);
			std::string data;

			std::getline(this->blackBoxFileStream, data);
			memcpy(pBuffer, data.c_str(), data.length());
			pBuffer += ALTITUDE_LEN;
			std::getline(this->blackBoxFileStream, data);
			memcpy(pBuffer, data.c_str(), data.length());
			pBuffer += POSITION_LEN;
			std::getline(this->blackBoxFileStream, data);
			memcpy(pBuffer, data.c_str(), data.length());
			pBuffer += SPEED_LEN;
			std::getline(this->blackBoxFileStream, data);
			memcpy(pBuffer, data.c_str(), data.length());

			entry.setData(pBuffer, data.length());
			entry.setSeqNum(0);
			entry.setTotalCount(1);

			this->packetizedBlackBoxData.push_back(entry);

		}
	}
	catch (...)
	{
		retVal = -1;
	}
	blackBoxFileStream.close();
	return retVal;
}

const std::vector<Packet>* BlackBox::getPacketList()
{
	return &this->packetizedBlackBoxData;
}
