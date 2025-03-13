#include "blackbox.hpp"
BlackBox::BlackBox()
{
}

int BlackBox::loadData(char* pathToImage)
{
	int retVal;

	retVal = 0;

	if (this->packetizedBlackBoxData.size())
	{
		this->packetizedBlackBoxData.clear()
	}


	//altitude, position, speed, heading
	try {
		this->blackBoxFileStream.open(path, std::fstream::in);
		while (this->blackBoxFileStream.is_open() && !this->blackBoxFileStream.eof())
		{
			Packet entry(MAX_BODY_LENGTH);
			char* data = entry.getData();

			std::getline(this->blackBoxFileStream, data);
			data += ALTITUDE_LEN;
			std::getline(this->blackBoxFileStream, data);
			data += POSITION_LEN;
			std::getline(this->blackBoxFileStream, data);
			data += SPEED_LEN;
			std::getline(this->blackBoxFileStream, data);
			
			this->packetizedBlackBoxData.push_back(entry);

		}
	}
	catch (...)
	{
		retVal = -1;
	}
	
	return retVal;
}

const std::vector<Packet>* BlackBox::getPacketList()
{
	return &this->packetizedBlackBoxData;
}
