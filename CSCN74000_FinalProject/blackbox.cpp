#include "blackbox.h"
BlackBox::BlackBox()
{
}

int BlackBox::loadData(char* pathToData)
{
	int retVal;
	int sequence;

	retVal = 0;
	sequence = 0;

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
			std::string data = entry.getData();

			std::getline(this->blackBoxFileStream, data);
			data += ALTITUDE_LEN;
			std::getline(this->blackBoxFileStream, data);
			data += POSITION_LEN;
			std::getline(this->blackBoxFileStream, data);
			data += SPEED_LEN;
			std::getline(this->blackBoxFileStream, data);

			entry.setData(data.c_str(), data.length());
			entry.setSeqNum(sequence);
			sequence++;

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
