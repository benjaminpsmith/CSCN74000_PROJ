#include <WinSock2.h>
#include <WS2tcpip.h>

#include <Windows.h>

#include <string.h>
#include <iostream>
#include <cstdint>
#include "packet.h"

namespace ConnectionData {

#define CONN_PORT 34214
#define SERVER_IP "127.0.0.1"
#define CLIENT_IP "127.0.0.1"
#define SERVER_PORT 34254
#define CLIENT_PORT 44254
#define SECURE_PASSWORD "lkjsdHJBFf987(*&%^bjsfy_SDGk187%^&$"
#define SECURE_PASSWORD_LEN 36

#define SERVER_ID 723764
#define AIRPLANE_ID 22367
#define AIRPLANE_ID_LEN 3

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
		int accept(PacketData::PacketDef& handshakePacket, address* targetAddress);
		address createAddress(port portNum, iPAddress ip = nullptr);
		int establishConnection(PacketData::PacketDef& handshakePacket, address* targetAddress);
		void setConnectionDetails(const fd* socketFd, const address* targetAddress);
		bool isAuthenticated();
		ConnState getAuthenticationState();
		void setPassphrase(const char* passphrase);

		~Connection();
	};
}