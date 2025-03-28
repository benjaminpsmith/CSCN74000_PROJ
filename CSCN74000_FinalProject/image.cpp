#include "image.h"

namespace WeatherImage
{
	Image::Image()
	{
		imgSize = 0;
	}

	int Image::loadImage(const char* pathToImage)
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
				pFile->pubseekpos(0, this->imageInFileStream.in);

				this->imgSize = imageSize;

				for (int i = 0; i < imageSize; i += packetizedLen)
				{
					if ((imageSize - i) < PacketData::Constants::MAX_BODY_LENGTH)
					{
						packetizedLen = (imageSize - i);
					}
					else
					{
						packetizedLen = PacketData::Constants::MAX_BODY_LENGTH;
					}
					char* buffer = new char[packetizedLen];

					bytesRead = pFile->sgetn(buffer, packetizedLen);
					
					if (bytesRead > 0)
					{
						PacketDef* entry = new PacketDef();

						entry->setSeqNum(sequence);
						entry->setFlag(PacketDef::Flag::IMG);
						entry->setData(buffer, bytesRead);
						entry->setCrc(0);

						sequence++;
						this->packetizedImage.push_back(entry);
					}

					delete[] buffer;
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

	const std::vector<PacketDef*>* Image::getPacketList()
	{
		return &this->packetizedImage;
	}

	int Image::getPacketCount()
	{
		return this->packetizedImage.size();
	}

	void Image::addSome(PacketDef& packet)
	{
		PacketDef* anotherPacket = new PacketDef();
		*anotherPacket = packet;
		this->packetizedImage.push_back(anotherPacket);
	}

	bool Image::saveImage(const char* pathToImage)
	{
		char* data = nullptr;

		this->imageOutFileStream.open(pathToImage, std::fstream::out | std::ios_base::binary);
		
		if (this->imageOutFileStream.is_open())
		{
			for (int i = 0; i < this->packetizedImage.size() - 1; i++)
			{
				data = packetizedImage.at(i)->getData();

				this->imageOutFileStream.write(data, packetizedImage.at(i)->getBodyLen());
			}

			this->imageOutFileStream.close();
		}

		return false;
	}

	Image::~Image()
	{
		for (int i = 0; i < this->packetizedImage.size(); i++)
		{
			delete this->packetizedImage.at(i);
		}
	}

}

