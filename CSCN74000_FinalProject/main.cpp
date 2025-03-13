// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "position.h"

#define SERVER_IP "10.144.98.141"
#define SERVER_PORT 34254

int main(void){

    ConnDetails connectionDetails;
    Connection flightConnection;
    PacketDef received;
    PacketDef toSend;
    bool shutdown;
    char recvBuffer[MAX_PACKET_LENGTH];
    address rxSender;
    int addrLength;
    int bytesRead;


    shutdown = false;
    bytesRead = 0;

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_IP, SERVER_PORT);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    flightConnection.bindTo(&connectionDetails.socket, &connectionDetails.addr);

    while (!shutdown)
    {
        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*) & rxSender, &addrLength);
            
            received = PacketDef(recvBuffer, bytesRead);
            flightConnection.accept(received, &rxSender);

            if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
            {
                Sleep(1);
            }
        }
        
        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);
        received = PacketDef(recvBuffer, bytesRead);

        if (received.getFlag() == PacketDef::Flag::BB)  // We are now receiving the black-box data from the client
        {
            // Convert the body into a string
			std::string data(received.getData(), received.getBodyLen());
			Position pos(data);
			std::string filename = to_string(received.getSrc()) + "_blackbox.csv";
			pos.writeToFile(filename);
        }

        if (received.getFlag() == PacketDef::Flag::IMG)
        {
            //receiving request for an image to send

        }

        if (bytesRead <= 0)
        {
            Sleep(1);
        }
    }

    return 1;
}