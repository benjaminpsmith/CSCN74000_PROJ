// Source file for main of the server
#include "packet.h"
#include "connection.h"

#define SERVER_IP "192.168.1.50"
#define SERVER_PORT 34254

int main(void){

    ConnDetails connectionDetails;
    Connection flightConnection;
    Packet received;
    Packet toSend;
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

    flightConnection.bindTo(&connectionDetails.socket, &connectionDetails.addr);

    while (!shutdown)
    {
        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*) & rxSender, &addrLength);
            
            received = Packet(recvBuffer, bytesRead);
            flightConnection.accept(received, &rxSender);

            if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
            {
                Sleep(1);
            }
        }
        
        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);
        received = Packet(recvBuffer, bytesRead);

        if (received.getFlag() == Packet::Flag::BB)
        {
            //receiving telemetry from the plane to record
        }

        if (received.getFlag() == Packet::Flag::IMG)
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