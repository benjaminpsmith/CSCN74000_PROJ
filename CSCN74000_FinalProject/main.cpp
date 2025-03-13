// Source file for main of the server
#include "packet.h"
#include "connection.hpp"

#define SERVER_IP "192.168.1.50"
#define SERVER_PORT = 34254;

int main(void){

    ConnDetails connectionDetails;
    Connection flightConnection;
    Packet received;
    Packet toSend;
    bool shutdown;
    uint8_t recvBuffer[MAX_PACKET_LENGTH];
    address rxSender;
    int rxLength;

    shutdown = false;
    rxLength = 0;
    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_IP, SERVER_PORT);
    flightConnection.setConnectionDetails(&connectionDetails.socket, (struct sockaddr*)&connectionDetails.addr);

    while (!shutdown)
    {
        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, &rxSender, &rxLength);
            
            received = Packet(recvBuffer, rxLength);
            flightConnection.accept(received, rxSender);
        }
        
        recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, &rxSender, &rxLength);
        received = Packet(recvBuffer, rxLength);

        if (received.getFlag() == Packet::Flag::BB)
        {
            //receiving telemetry from the plane to record
        }

        if (received.getFlag() == Packet::Flag::IMG)
        {
            //receiving request for an image to send
        }

    }

    return 1;
}