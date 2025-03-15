// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "blackbox.h"
#include "position.h"

int sendBlackBoxData(PacketDef& toSend, PacketDef& received, struct ConnDetails* pConnDets, sockaddr* rxSender, int* addrLength, char* recvBuffer);


int main(void) {

    ConnDetails connectionDetails;
    Connection flightConnection;
    PacketDef received;
    PacketDef toSend;
    bool shutdown;
    char recvBuffer[MAX_PACKET_LENGTH];
    address rxSender;
    int addrLength;
    int bytesRead;
    PacketDef beginsHandshake;
    int err;
 

    shutdown = false;
    bytesRead = 0;
    addrLength = 0;
    err = 0;
    
    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_IP, SERVER_PORT);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    addrLength = sizeof(rxSender);

    beginsHandshake.setData(SECURE_PASSWORD, strlen(SECURE_PASSWORD));
    beginsHandshake.setDest(SERVER_ID);
    beginsHandshake.setSeqNum(0);
    beginsHandshake.setSrc(AIRPLANE_ID);
    beginsHandshake.setTotalCount(1);


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

                err = WSAGetLastError();

                if (bytesRead > 0)
                {
                    received = PacketDef(recvBuffer, bytesRead);
                    flightConnection.establishConnection(received, &rxSender);
                }
                
            }
            

            if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
            {
                Sleep(1);
            }
        }

        // Final ACK?
        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);

        if (bytesRead > 0)
        {
            received = PacketDef(recvBuffer, bytesRead);


            if (received.getFlag() == PacketDef::Flag::BB)  // The server has requested the client to send the black-box data
            {

                sendBlackBoxData(toSend, received, &connectionDetails, (struct sockaddr*)&rxSender, &addrLength, recvBuffer);
                
            }

			if (received.getFlag() == PacketDef::Flag::IMG) // The client has previously requested an image, and it is now being delivered.
			{
				// We need to store all the packets that will be used to reconstruct the image
			}
        }


        if (bytesRead <= 0)
        {
            Sleep(1);
        }
    }

    return 1;
}

int sendBlackBoxData(PacketDef& toSend, PacketDef& received, struct ConnDetails* pConnDets, sockaddr* rxSender, int* addrLength, char* recvBuffer)
{
    int bytesRead = 0;
    bool ret = false;
    Position pos;
    pos.createRandomValues();                       // Create the fake position data
    std::string data = pos.createStringToSend();    // Convert the position data to a string

    toSend.setData(data.c_str(), data.length());    // Set the body and body length of the new packet
    if (toSend.getData() == nullptr)
    {
        std::cout << "Error setting data, size too large or error allocating memory." << std::endl;
        // Throw error?
    }
    char* buffer = nullptr;
    unsigned int totalSize = toSend.Serialize(buffer);
    if (buffer != nullptr)
        send(pConnDets->socket, buffer, totalSize, NULL); // Send the packet

    // Check for ACK
    bytesRead = recvfrom(pConnDets->socket, recvBuffer, MAX_PACKET_LENGTH, NULL, rxSender, addrLength);
    received = PacketDef(recvBuffer, bytesRead);
    if (received.getFlag() != PacketDef::Flag::ACK)
    {
        // Error and no ACK returned
        //send again??? or abort transmission - would require tasking another flag bit for abort
    }

    return ret;
}
