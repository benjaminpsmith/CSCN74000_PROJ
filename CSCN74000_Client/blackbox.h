#include "packet.h"
#include <vector>
#include <fstream>
#include <stdio.h>
#include <string>

#define ALTITUDE_LEN 10
#define POSITION_LEN 10
#define SPEED_LEN 10

class BlackBox : public  Packet
{
	std::vector<Packet> packetizedBlackBoxData;
	std::fstream blackBoxFileStream;
	char* filePath;
public:

	BlackBox();
	int loadData(char* pathToImage);

	const std::vector<Packet>* getPacketList();
};