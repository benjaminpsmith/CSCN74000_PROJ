
#define CONN_PORT 34214
#define SERVER_IP "10.144.122.216"
#define SERVER_PORT 34254
#define SECURE_PASSWORD "lkjsdHJBFf987(*&%^bjsfy_SDGk187%^&$"

#define SERVER_ID 723764
#define AIRPLANE_ID 22367
#define AIRPLANE_ID_LEN 3

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
typedef SOCKET fd;

typedef enum CONNECTION_STATE : int8_t
{
	UNAUTHENTICATED,
	HANDSHAKING,
	AUTHENTICATED
}ConnState;

struct ConnDetails {
	fd socket;
	address addr;
	char airplaneID[AIRPLANE_ID_LEN];
};

class Connection {
private:
	WSADATA wsaData;
	ConnState state;
	const char* passphrase;
	ConnDetails connectionDetails;

public:
	Connection();

	fd createSocket();
	int bindTo(fd* socketFd, address* targetAddress);
	int accept(PacketDef& handshakePacket, address* targetAddress);
	address createAddress(iPAddress ip, port portNum);
	int establishConnection(PacketDef& handshakePacket, address* targetAddress);
	void setConnectionDetails(fd* socketFd, address* targetAddress);
	bool isAuthenticated();
	ConnState getAuthenticationState();
	void setPassphrase(const char* passphrase);

	~Connection();
};
