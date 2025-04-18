#include "connection.h"

namespace ConnectionData {

	Connection::Connection()
	{
		state = ConnState::UNAUTHENTICATED;
		WORD version = MAKEWORD(2, 2);
		int err = WSAStartup(version, &wsaData);
		if (err)
		{
			std::cerr << "Error initializing WSA." << std::endl;
		}

		this->connectionDetails.socket = 0;
		this->passphrase = nullptr;
		if (memset(&this->wsaData, 0, sizeof(wsaData)) != &this->wsaData)
		{
			std::cerr << "Error initializing wsaData." << std::endl;
		}

	}

	fd Connection::createSocket()
	{
		fd createdSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (createdSocket < 0)
		{
			int err = WSAGetLastError();
			std::cerr << "Error creating socket." << std::endl;
			int cleanupValue = WSACleanup();
			std::cout << "WSACleanup returned: " << cleanupValue << std::endl;
		}
		return createdSocket;
	}

	address Connection::createAddress(port portNum, iPAddress ip)
	{
		address connectionAddress;

		connectionAddress.sin_family = AF_INET;
		connectionAddress.sin_port = htons(portNum);
		if (ip == nullptr)
		{
			connectionAddress.sin_addr.s_addr = INADDR_ANY;
		}
		else
		{
			connectionAddress.sin_addr.s_addr = inet_addr(ip);
		}


		return connectionAddress;
	}

	int Connection::establishConnection(PacketData::PacketDef& handshakePacket, address* targetAddress)
	{
		int ret;
		uint8_t flags;
		int authBit;
		int ackBit;
		int restartAuth;
		char buffer[PacketData::Constants::MAX_PACKET_LENGTH];
		
		restartAuth = 0;
		ret = 0;
		flags = handshakePacket.getFlag();

		authBit = flags & PacketData::PacketDef::Flag::AUTH;
		ackBit = flags & PacketData::PacketDef::Flag::ACK;
		restartAuth = flags & PacketData::PacketDef::Flag::AUTH_LOST;

		if (state == ConnState::UNAUTHENTICATED && authBit != PacketData::PacketDef::Flag::AUTH)
		{
			handshakePacket.setFlag(PacketData::PacketDef::Flag::AUTH);
			ret = handshakePacket.Serialize(&buffer[0]);
			 
			if (ret)
			{
				ret = sendto(connectionDetails.socket, &buffer[0], ret, 0, reinterpret_cast<struct sockaddr*>(targetAddress), sizeof(*targetAddress));
				int err = WSAGetLastError();
			}
			state = ConnState::HANDSHAKING;
		} 

		if (state == ConnState::HANDSHAKING && authBit == PacketData::PacketDef::Flag::AUTH && ackBit == PacketData::PacketDef::Flag::ACK)
		{
			handshakePacket.setFlag(PacketData::PacketDef::Flag::ACK);
			ret = handshakePacket.Serialize(&buffer[0]);

			if (ret) 
			{
				ret = sendto(connectionDetails.socket, &buffer[0], ret, 0, reinterpret_cast<struct sockaddr*>(targetAddress), sizeof(*targetAddress));
			}

		}

		if (state == ConnState::HANDSHAKING && ackBit == PacketData::PacketDef::Flag::ACK && authBit != PacketData::PacketDef::Flag::AUTH)
		{
			state = ConnState::AUTHENTICATED;
			ret = 1;
		}

		if (restartAuth == PacketData::PacketDef::Flag::AUTH_LOST)
		{
			state = ConnState::UNAUTHENTICATED;
		}

		return ret;
	}

	void Connection::setConnectionDetails(const fd* socketFd, const address* targetAddress)
	{
		this->connectionDetails.socket = *socketFd;
		this->connectionDetails.addr = *targetAddress;
	}

	bool Connection::isAuthenticated()
	{
		return this->state == ConnState::AUTHENTICATED;
	}

	ConnState Connection::getAuthenticationState()
	{
		return this->state;
	}

	void Connection::setPassphrase(const char* passphrase)
	{
		this->passphrase = passphrase;
	}

	void Connection::restartAuth()
	{
		this->state = ConnState::UNAUTHENTICATED;
	}

	int Connection::bindTo(const fd* socketFd, address* targetAddress)
	{
		int retValue = 1;

		if (bind(*socketFd, reinterpret_cast<struct sockaddr*>(targetAddress), sizeof(sockaddr)) < 0)
		{
			int err = WSAGetLastError();
			std::cerr << "Error binding to socket." << std::endl;
			retValue = 0;
		}

		return retValue;
	}

	int Connection::accept(PacketData::PacketDef& handshakePacket, address* targetAddress)
	{
		bool ret;
		uint8_t flags;
		int authBit;
		int ackBit;
		char* authData;
		uint32_t airplaneID;
		int bytes;

		char buffer[PacketData::Constants::MAX_PACKET_LENGTH];

		bytes = 0;
		airplaneID = 0;
		authData = nullptr;
		ret = 0;
		authBit = 0;
		ackBit = 0;
		flags = handshakePacket.getFlag();

		authBit = flags & 0x4;
		ackBit = flags & 0x8;

		if (state == ConnState::UNAUTHENTICATED && authBit == PacketData::PacketDef::Flag::AUTH)
		{
			//extract the password and compare it to the correct one

			authData = handshakePacket.getData();

			if (authData != nullptr)
			{
				bytes = strncmp(authData, this->passphrase, handshakePacket.getBodyLen());
			}

			if (bytes == 0)
			{
				//send AUTH + ACK
				handshakePacket.setFlag(PacketData::PacketDef::Flag::AUTH_ACK);
				bytes = handshakePacket.Serialize(&buffer[0]);

				if (bytes > 0)
				{
					bytes = sendto(connectionDetails.socket, &buffer[0], bytes, 0, reinterpret_cast<struct sockaddr*>(targetAddress), sizeof(*targetAddress));
				}
				airplaneID = handshakePacket.getSrc();

				if (memcpy(static_cast<void*>(&this->connectionDetails.airplaneID), static_cast<void*>(&airplaneID), 3) == nullptr)
				{
					std::cerr << "Error copying airplane ID." << std::endl;
				}

				state = ConnState::HANDSHAKING;
			}

		}

		if (state == ConnState::HANDSHAKING && ackBit == PacketData::PacketDef::Flag::ACK  && authBit !=  PacketData::PacketDef::Flag::AUTH)
		{

			//send ACK
			handshakePacket.setFlag(PacketData::PacketDef::Flag::ACK);
			bytes = handshakePacket.Serialize(&buffer[0]);

			if (bytes > 0)
			{
				bytes = sendto(connectionDetails.socket, &buffer[0], bytes, 0, reinterpret_cast<struct sockaddr*>(targetAddress), sizeof(*targetAddress));
			}

			state = ConnState::AUTHENTICATED;
		}

		return 1;
	}

	Connection::~Connection()
	{
	}

}