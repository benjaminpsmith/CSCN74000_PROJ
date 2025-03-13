// Source file for main of the server
#include "..\CSCN74000_FinalProject\packet.h"
#include "..\CSCN74000_FinalProject\connection.h"
#include "..\CSCN74000_FinalProject\blackbox.h"
#include <thread>

int main(void) {

    ConnDetails connectionDetails;
    Connection flightConnection;
    Packet received;
    Packet toSend;
    bool shutdown;
    char recvBuffer[MAX_PACKET_LENGTH];
    address rxSender;
    int addrLength;
    int bytesRead;
    Packet beginsHandshake;
    std::thread timerThread;
    bool secondElapsed;

    secondElapsed = false;
    shutdown = false;
    bytesRead = 0;
    addrLength = 0;

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_IP, SERVER_PORT);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    beginsHandshake.setData(SECURE_PASSWORD, strlen(SECURE_PASSWORD));
    beginsHandshake.setDest(SERVER_ID);
    beginsHandshake.setSeqNum(0);
    beginsHandshake.setSrc(AIRPLANE_ID);
    beginsHandshake.setTotalCount(1);

    //set up timer to set variable we can use to determine if a second has elapsed
   
    timerThread = std::thread([&] {
        Sleep(500);
        secondElapsed = true;
        Sleep(500);
        secondElapsed = false;
        });

    timerThread.detach();

    while (!shutdown)
    {
        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            if (flightConnection.getAuthenticationState() == ConnState::UNAUTHENTICATED)
            {
                //start the connection handshake
                flightConnection.establishConnection(beginsHandshake, &connectionDetails.addr);
            }
            else
            {
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);

                received = Packet(recvBuffer, bytesRead);
                flightConnection.establishConnection(received, &rxSender);
            }
            

            if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
            {
                Sleep(1);
            }
        }

        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);

        if (bytesRead > 0)
        {
            received = Packet(recvBuffer, bytesRead);


            if (received.getFlag() == Packet::Flag::IMG)
            {
                //receiving response with an image
            }
        }
        
        if (secondElapsed)
        {
            //send BB data to ground control
            //reqquest image

        }

        if (bytesRead <= 0)
        {
            Sleep(1);
        }
    }

    return 1;
}