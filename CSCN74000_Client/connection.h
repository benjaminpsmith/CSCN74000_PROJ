#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#define CONN_PORT 34214

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <string.h>
#include <iostream>
#include <cstdint>
#include <string.h>
#include "packet.h"

#pragma comment(lib, "ws2_32")

typedef struct sockaddr_in address;
typedef const char* iPAddress;
typedef uint16_t port;
typedef int fd;

typedef enum CONNECTION_STATE : int8_t
{
	UNAUTHENTICATED,
	HANDSHAKING,
	AUTHENTICATED
}ConnState;

struct ConnDetails {
	fd socket;
	address addr;
};

class Connection {
private:
	WSADATA wsaData;
	ConnState state;
	char* passphrase;
	ConnDetails connectionDetails;

public:
	Connection();

	fd createSocket();
	int bind(fd* socketFd, address* targetAddress);
	int accept(Packet& handshakePacket, address* targetAddress);
	address createAddress(iPAddress ip, port portNum);
	int establishConnection(Packet& handshakePacket, address* targetAddress);
	void setConnectionDetails(fd* socketFd, address* targetAddress);
	bool isAuthenticated();
	ConnState getAuthenticationState();

	~Connection();
};

#endif