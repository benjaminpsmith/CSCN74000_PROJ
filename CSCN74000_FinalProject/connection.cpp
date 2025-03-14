#include "connection.h"

Connection::Connection()
{
	WORD version = MAKEWORD(2, 2);
	int err = WSAStartup(version, &wsaData);

	this->connectionDetails.socket = 0;
	this->passphrase = nullptr;
	memset(&this->wsaData, 0, sizeof(wsaData));

}

fd Connection::createSocket()
{
	fd createdSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (createdSocket < 0)
	{
		int err = WSAGetLastError();
		perror("Error creating socket.\n");
		WSACleanup();
	}
	return createdSocket;
}

address Connection::createAddress(iPAddress ip, port portNum)
{
	address connectionAddress;

	connectionAddress.sin_family = AF_INET;
	connectionAddress.sin_port = htons(portNum);
	connectionAddress.sin_addr.s_addr = INADDR_ANY;

	return connectionAddress;
}

int Connection::establishConnection(PacketDef& handshakePacket, address* targetAddress)
{
	int ret;
	uint8_t flags;
	int authBit;
	int ackBit;
	char buffer[MAX_PACKET_LENGTH];

	ret = 0;
	flags = handshakePacket.getFlag();

	authBit = flags & 0x8;
	ackBit = flags & 0x10;

	if (state == ConnState::UNAUTHENTICATED && !authBit)
	{
		handshakePacket.setFlag(PacketDef::Flag::AUTH);
		ret = handshakePacket.Serialize(buffer);

		if (ret)
		{
			ret = sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, sizeof(*targetAddress));
		}
		state = ConnState::HANDSHAKING;
	}

	if (state == ConnState::HANDSHAKING && authBit && ackBit)
	{
		handshakePacket.setFlag(PacketDef::Flag::ACK);
		ret = handshakePacket.Serialize(buffer);

		if (ret)
		{
			ret = sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, sizeof(*targetAddress));
		}
		state = ConnState::AUTHENTICATED;
	}

	if (state == ConnState::AUTHENTICATED && ackBit)
	{
		ret = -1;
	}

	return ret;
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

int Connection::accept(PacketDef& handshakePacket, address* targetAddress)
{
	int ret;
	uint8_t flags;
	int authBit;
	int ackBit;
	char* authData;

	char buffer[MAX_PACKET_LENGTH];

	authData = nullptr;
	ret = 0;
	authBit = 0;
	ackBit = 0;
	flags = handshakePacket.getFlag();

	authBit = flags & 0x8;
	ackBit = flags & 0x10;

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
			handshakePacket.setFlag(PacketDef::Flag::AUTH_ACK);
			ret = handshakePacket.Serialize(buffer);

			if (ret)
			{
				ret = sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, sizeof(*targetAddress));
			}
			state = ConnState::HANDSHAKING;
		}

	}

	if (state == ConnState::HANDSHAKING && ackBit && !authBit)
	{
		state = ConnState::AUTHENTICATED;
	}

	if (state == ConnState::AUTHENTICATED)
	{
		//send ACK
		handshakePacket.setFlag(PacketDef::Flag::ACK);
		ret = handshakePacket.Serialize(buffer);

		if (ret)
		{
			ret = sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, sizeof(*targetAddress));
		}

		ret = 1;
	}

	return ret;
}

Connection::~Connection()
{
}
