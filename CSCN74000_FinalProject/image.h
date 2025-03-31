#ifndef HPP_IMAGE
#define HPP_IMAGE

#include <vector>
#include <fstream>
#include "packet.h"


#define DEFAULT_IMAGE_PATH "image.png"
#define MAX_IMAGE_CHUNK_SIZE 255 //10mb


namespace WeatherImage {

	class Image
	{
		std::vector<PacketData::PacketDef*> packetizedImage;
		std::ifstream imageInFileStream;
		std::ofstream imageOutFileStream;
		int imgSize;

	public:

		Image()
		{
			imgSize = 0;
		}

		int loadImage(const char* pathToImage = DEFAULT_IMAGE_PATH)
		{
			int retVal;
			size_t imageSize;
			std::streamsize bytesRead;
			int packetizedLen;
			int sequence;
			std::filebuf* pFile = nullptr;
			retVal = 0;
			imageSize = 0;
			packetizedLen = 0;
			sequence = 1;
			bytesRead = 0;
			const int length = PacketData::Constants::MAX_BODY_LENGTH;
			char buffer[length];
			std::streampos pos;

			pos = 0;

			if (this->packetizedImage.size())
			{
				this->packetizedImage.clear();
			}

			//altitude, position, speed, heading
			try {
				this->imageInFileStream.open(pathToImage, std::ios_base::in | std::ios_base::binary);

				if (this->imageInFileStream.is_open())
				{
					pFile = this->imageInFileStream.rdbuf();

					imageSize = pFile->pubseekoff(0, this->imageInFileStream.end, this->imageInFileStream.in);
					pos = pFile->pubseekpos(0, this->imageInFileStream.in);

					this->imgSize = imageSize;

					for (int i = pos; i < imageSize; i += packetizedLen)
					{
						if ((imageSize - i) < PacketData::Constants::MAX_BODY_LENGTH)
						{
							packetizedLen = (imageSize - i);
							bytesRead = pFile->sgetn(&buffer[0], packetizedLen);
						}
						else
						{
							packetizedLen = PacketData::Constants::MAX_BODY_LENGTH;
							bytesRead = pFile->sgetn(&buffer[0], packetizedLen);
						}

						if (bytesRead > 0)
						{
							PacketData::PacketDef* entry = new PacketData::PacketDef();

							entry->setSeqNum(sequence);
							entry->setFlag(PacketData::PacketDef::Flag::IMG);
							if (entry->setData(&buffer[0], bytesRead) > 0)
							{
								//if the CRC doesnt get set the packet will get dropped
								entry->setCrc(0);
							}
							
							sequence++;
							this->packetizedImage.push_back(entry);
						}
					}

					for (int i = 0; i < this->packetizedImage.size(); i++)
					{
						this->packetizedImage.at(i)->setTotalCount(this->packetizedImage.size());
					}
				}
			}
			catch (...)
			{
				retVal = -1;
			}

			return retVal;
		}

		const std::vector<PacketData::PacketDef*>* getPacketList()
		{
			return &this->packetizedImage;
		}

		int getPacketCount()
		{
			return this->packetizedImage.size();
		}

		void addSome(PacketData::PacketDef& packet)
		{
			PacketData::PacketDef* anotherPacket = new PacketData::PacketDef();
			*anotherPacket = packet;
			this->packetizedImage.push_back(anotherPacket);
		}

		bool saveImage(const char* pathToImage = DEFAULT_IMAGE_PATH)
		{
			char* data = nullptr;
			bool ret = false;

			this->imageOutFileStream.open(pathToImage, std::fstream::out | std::ios_base::binary);

			if (this->imageOutFileStream.is_open())
			{
				for (int i = 0; i < this->packetizedImage.size(); i++)
				{
					data = packetizedImage.at(i)->getData();

					if (this->imageOutFileStream.write(data, packetizedImage.at(i)->getBodyLen()))
					{
						ret = true;
					}
				}

				this->imageOutFileStream.close();
			}

			return ret;
		}

		~Image()
		{
			for (int i = 0; i < this->packetizedImage.size(); i++)
			{
				delete this->packetizedImage.at(i);
			}
		}
	};

}

#endif