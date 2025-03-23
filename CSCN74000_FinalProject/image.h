#ifndef HPP_IMAGE
#define HPP_IMAGE

#include <vector>
#include <fstream>
#include "packet.h"


#define DEFAULT_IMAGE_PATH "image.png"
#define MAX_IMAGE_CHUNK_SIZE 255 //10mb

using namespace PacketData;


namespace WeatherImage {

	class Image
	{
		std::vector<PacketDef*> packetizedImage;
		std::ifstream imageInFileStream;
		std::ofstream imageOutFileStream;
		int imgSize;

	public:

		Image();
		int loadImage(const char* pathToImage = DEFAULT_IMAGE_PATH);
		const std::vector<PacketDef*>* getPacketList();
		int getPacketCount();
		void addSome(PacketDef& packet);
		bool saveImage(const char* = DEFAULT_IMAGE_PATH);

		~Image();
	};

}

#endif