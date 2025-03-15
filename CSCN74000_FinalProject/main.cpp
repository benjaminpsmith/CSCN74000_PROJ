// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "position.h"
#include <thread>

#define SERVER_IP "10.144.98.141"
#define SERVER_PORT 34254

#define BLACKBOX_FILE "_blackbox.csv"

typedef enum SERVER_STATE : int8_t
{
    AWAITING_AUTH,
    IDLE,
    PROCESSING,
    SENDING,
    RECEIVING
}ServerState;

int serverThread(PacketDef&, bool, int);


int main(void){
    PacketDef received;
    bool firstHandshakePacket = false;

    serverThread(received, firstHandshakePacket, SERVER_PORT);
}

int serverThread(PacketDef& received, bool firstHandshakePacket, int serverPort)
{
    ServerState state;
    ConnDetails connectionDetails;
    Connection flightConnection;
    PacketDef toSend;
    bool shutdown;
    char recvBuffer[MAX_PACKET_LENGTH];
    address rxSender;
    int addrLength;
    int bytesRead;
    std::thread timerThread;
    bool secondElapsed;

    state = AWAITING_AUTH;
    secondElapsed = false;
    shutdown = false;
    bytesRead = 0;

    //set up timer to set variable we can use to determine if a second has elapsed

    timerThread = std::thread([&] {
        Sleep(500);
        secondElapsed = true;
        Sleep(500);
        secondElapsed = false;
        });


    timerThread.detach();

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_IP, serverPort);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    flightConnection.bindTo(&connectionDetails.socket, &connectionDetails.addr);

    //server states:
    //awaiting, idle, processing, sending or receiving


    while (!shutdown)
    {
        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            state = AWAITING_AUTH;

            //Only the main thread would not have a packet provided to it. Other threads will have an auth bit set in a packet passed to it as a result of the parent thread receiving a packet with an auth bit.
            //The packet is passed to the new thread, and that new thread will use that packet as the starting point for the handshake
            if (!firstHandshakePacket)
            {
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);
                received = PacketDef(recvBuffer, bytesRead);
            }
            
            flightConnection.accept(received, &rxSender);

            if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
            {
                Sleep(1);
            }

        }

        //initially this is 0, if nothing is received it is -1, and only if we receive do we leave the idle state. We go back to the idle state after 1 instance of receiving nothing.
        if (bytesRead == 0)
        {
            state = ServerState::IDLE;
        }

        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);
        received = PacketDef(recvBuffer, bytesRead);    //RECV BB DATA


        if (received.getFlag() == PacketDef::Flag::BB && state == ServerState::IDLE)  // We are now receiving the black-box data from the client
        {
            state = ServerState::RECEIVING;
            std::string data(received.getData(), received.getBodyLen());    // Convert the body into a string
            Position pos(data);	                                            // Create a position object from the string     
            std::string filename = to_string(received.getSrc()) + BLACKBOX_FILE;    // Create the filename for the black-box data
            pos.writeToFile(filename);	                                            // Write the black-box data to a file named "clientID_blackbox.csv"

            // Send an ACK back to the client -  could potentially make one ahead of time and use it repeatedly since the body is empty, would only need to change dest?
            toSend.setSrc(SERVER_ID);
            toSend.setDest(received.getSrc());
            toSend.setFlag(PacketDef::Flag::ACK);
            toSend.setSeqNum(1);
            toSend.setTotalCount(1);
            toSend.setBodyLen(0);
            toSend.setData(nullptr, 0);
            toSend.setCrc(0);

            char sendBuffer[MAX_PACKET_LENGTH];
            int bytesToSend = toSend.Serialize(sendBuffer);  // SEND ACK
            sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, (struct sockaddr*)&rxSender, addrLength);


        }

        if (received.getFlag() == PacketDef::Flag::IMG && state == ServerState::IDLE)
        {
            //receiving request for an image to send

            //get the image from file

            while (1)//some appropriate condition like count of increments of full packets + 1 with remainder
            {
                state = ServerState::PROCESSING;
                //break up the image


                //transition to sending and stay in sending until all are sent - will go back to idle after 1 receive without a packet 
                state = ServerState::SENDING;

                break;//remove
            }

        }

        if (secondElapsed)
        {
            //send request for bb data

            toSend.setSrc(SERVER_ID);
            toSend.setDest(received.getSrc());
            toSend.setFlag(PacketDef::Flag::BB);
            toSend.setSeqNum(1);
            toSend.setTotalCount(1);
            toSend.setBodyLen(0);
            toSend.setData(nullptr, 0);
            toSend.setCrc(0);

            char sendBuffer[MAX_PACKET_LENGTH];
            int bytesToSend = toSend.Serialize(sendBuffer);  // SEND ACK
            sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, (struct sockaddr*)&rxSender, addrLength);
        }


        //If we receive an auth packet after establishing a n=connection with a client, start the whole process over
        //skip the first receive for an auth packet, because we have one.
        //detach so we dont have to cleanup after ourselves.
        if (received.getFlag() == PacketDef::AUTH)
        {
            std::thread newServerThread;

            newServerThread = std::thread([&] {

                serverThread(received, true, serverPort + 1);

                });

            newServerThread.detach();
        }


        if (bytesRead <= 0)
        {
            state = ServerState::IDLE;
            Sleep(1);
        }

    }

    return 1;
}
