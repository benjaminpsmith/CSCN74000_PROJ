// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "position.h"

#define SERVER_IP "10.144.98.141"
#define SERVER_PORT 34254

#define BLACKBOX_FILE "_blackbox.csv"

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
        received = PacketDef(recvBuffer, bytesRead);    //RECV BB DATA

        if (received.getFlag() == PacketDef::Flag::BB)  // We are now receiving the black-box data from the client
        {
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