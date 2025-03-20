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
		char buffer[PacketData::Constants::MAX_PACKET_LENGTH];

		ret = 0;
		flags = handshakePacket.getFlag();

		authBit = flags & 0x4;
		ackBit = flags & 0x8;

		if (state == ConnState::UNAUTHENTICATED && !authBit)
		{
			handshakePacket.setFlag(PacketData::PacketDef::Flag::AUTH);
			ret = handshakePacket.Serialize(buffer);

			if (ret)
			{
				ret = sendto(connectionDetails.socket, buffer, ret, 0, (struct sockaddr*)targetAddress, sizeof(*targetAddress));
				int err = WSAGetLastError();
			}
			state = ConnState::HANDSHAKING;
			return 1;
		}

		if (state == ConnState::HANDSHAKING && authBit && ackBit)
		{
			handshakePacket.setFlag(PacketData::PacketDef::Flag::ACK);
			ret = handshakePacket.Serialize(buffer);

			if (ret)
			{
				ret = sendto(connectionDetails.socket, buffer, ret, 0, (struct sockaddr*)targetAddress, sizeof(*targetAddress));
			}

			return 1;
		}

		if (state == ConnState::HANDSHAKING && ackBit)
		{
			state = ConnState::AUTHENTICATED;
			ret = 1;
		}

		return 1;
	}

	void Connection::setConnectionDetails(fd* socketFd, address* targetAddress)
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

	int Connection::bindTo(fd* socketFd, address* targetAddress)
	{
		if (bind(*socketFd, (struct sockaddr*)targetAddress, sizeof(sockaddr)) < 0)
		{
			int err = WSAGetLastError();
			perror("Error binding socket.\n");
			return 0;
		}

		return 1;
	}

	int Connection::accept(PacketData::PacketDef& handshakePacket, address* targetAddress)
	{
		int ret;
		uint8_t flags;
		int authBit;
		int ackBit;
		char* authData;
		uint32_t airplaneID;

		char buffer[PacketData::Constants::MAX_PACKET_LENGTH];

		airplaneID = 0;
		authData = nullptr;
		ret = 0;
		authBit = 0;
		ackBit = 0;
		flags = handshakePacket.getFlag();

		authBit = flags & 0x4;
		ackBit = flags & 0x8;

		if (state == ConnState::UNAUTHENTICATED && authBit)
		{
			//extract the password and compare it to the correct one

			authData = handshakePacket.getData();

			if (authData != nullptr)
			{
				ret = strncmp(authData, this->passphrase, handshakePacket.getBodyLen());
			}

			if (!ret)
			{
				//send AUTH + ACK
				handshakePacket.setFlag(PacketData::PacketDef::Flag::AUTH_ACK);
				ret = handshakePacket.Serialize(buffer);

				if (ret)
				{
					ret = sendto(connectionDetails.socket, buffer, ret, 0, (struct sockaddr*)targetAddress, sizeof(*targetAddress));
				}
				airplaneID = handshakePacket.getSrc();
				memcpy(this->connectionDetails.airplaneID, &airplaneID, 3);

				state = ConnState::HANDSHAKING;
			}

			return 1;

		}

		if (state == ConnState::HANDSHAKING && ackBit && !authBit)
		{

			//send ACK
			handshakePacket.setFlag(PacketData::PacketDef::Flag::ACK);
			ret = handshakePacket.Serialize(buffer);

			if (ret)
			{
				ret = sendto(connectionDetails.socket, buffer, ret, 0, (struct sockaddr*)targetAddress, sizeof(*targetAddress));
			}

			state = ConnState::AUTHENTICATED;

			return 1;
		}

		return 1;
	}

	Connection::~Connection()
	{
	}

}