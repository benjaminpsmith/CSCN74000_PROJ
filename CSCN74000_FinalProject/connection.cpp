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
	fd createdSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (createdSocket < 0)
	{
		int err = WSAGetLastError();
		perror("Error creating socket. ");
		WSACleanup();
	}
	return createdSocket;
}

address Connection::createAddress(iPAddress ip, port portNum)
{
	address connectionAddress;

	connectionAddress.sin_family = AF_INET;
	connectionAddress.sin_port = htons(portNum);
	inet_pton(AF_INET, ip, &connectionAddress);

	return connectionAddress;
}

int Connection::establishConnection(Packet& handshakePacket, address* targetAddress)
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
		handshakePacket.setFlag(Packet::Flag::AUTH);
		ret = handshakePacket.Serialize(buffer, MAX_PACKET_LENGTH);

		if (ret)
		{
			ret = sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, ret);
		}
		state = ConnState::HANDSHAKING;
	}

	if (state == ConnState::HANDSHAKING && authBit && ackBit)
	{
		handshakePacket.setFlag(Packet::Flag::ACK);
		ret = handshakePacket.Serialize(buffer, MAX_PACKET_LENGTH);

		if (ret)
		{
			ret = sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, ret);
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

int Connection::bind(fd* socketFd, address* targetAddress)
{
	return 0;
}

int Connection::accept(Packet& handshakePacket, address* targetAddress)
{
	int ret;
	uint8_t flags;
	int authBit;
	int ackBit;
	char* passwordData;

	char buffer[MAX_PACKET_LENGTH];

	passwordData = nullptr;
	ret = 0;
	authBit = 0;
	ackBit = 0;
	flags = handshakePacket.getFlag();

	authBit = flags & 0x8;
	ackBit = flags & 0x10;

	if (state == ConnState::UNAUTHENTICATED && authBit)
	{
		//extract the password and compare it to the correct one

		passwordData = handshakePacket.getData();

		if (passwordData != nullptr)
		{
			ret = strncmp(passwordData, this->passphrase, handshakePacket.getBodyLen());
		}

		if (!ret)
		{
			//send AUTH + ACK
			handshakePacket.setFlag(Packet::Flag::AUTH_ACK);
			ret = handshakePacket.Serialize(buffer, MAX_PACKET_LENGTH);

			if (ret)
			{
				ret = ::sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, ret);
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
		handshakePacket.setFlag(Packet::Flag::ACK);
		ret = handshakePacket.Serialize(buffer, MAX_PACKET_LENGTH);

		if (ret)
		{
			ret = ::sendto(connectionDetails.socket, buffer, ret, IPPROTO_UDP, (const sockaddr*)targetAddress, ret);
		}

		ret = 1;
	}

	return ret;
}

Connection::~Connection()
{
}
