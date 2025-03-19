// Source file for main of the server
#include "packet.h"
#include "connection.h"

#include <thread>

int main(void) {

    ConnDetails connectionDetails;
    Connection flightConnection;
    PacketDef received;
    PacketDef toSend;
    bool shutdown;
    char recvBuffer[Constants::MAX_PACKET_LENGTH];
    address rxSender;
    int addrLength;
    int bytesRead;
    PacketDef beginsHandshake;
    int err;
    std::thread timerThread;
    bool secondElapsed;

    secondElapsed = false;
    shutdown = false;
    bytesRead = 0;
    addrLength = 0;
    err = 0;
    memset(&connectionDetails.addr, 0, sizeof(connectionDetails.addr));
    memset(&rxSender, 0, sizeof(rxSender));


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
    connectionDetails.addr = flightConnection.createAddress(SERVER_PORT, SERVER_IP);
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
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, Constants::MAX_PACKET_LENGTH, NULL, (struct sockaddr*)&rxSender, &addrLength);

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

        // Client has now successfully been authenticated.
        // The client will now alternate between sending black box data to the server and requests for images.
		std::cout << " Client has been authenticated." << std::endl;
		std::cout << " Client will now alternate between sending black box data to the server and requests for images.\n" << std::endl;

		while (flightConnection.getAuthenticationState() == ConnState::AUTHENTICATED)   // Loop
		{
            bool send_blackbox_data = true;

            

			if (send_blackbox_data) { // Send black box data

                // Create buffers to hold the serialized position data and the serialized packet
                char posBuff[Constants::MAX_PACKET_LENGTH] = { 0 };
                char buffer [Constants::MAX_PACKET_LENGTH] = { 0 };
                
                // Generate a current position (currently random for proof of concept)
                Position currentPosition;
                currentPosition.createRandomValues();
                int positionLength = currentPosition.Serialize(posBuff);
				std::cout << currentPosition.latitude << " " << currentPosition.longitude << " " << currentPosition.heading << " " << currentPosition.velocity << " " << currentPosition.altitude << std::endl;


                // Construct the packet to send
                PacketDef blackbox_data(AIRPLANE_ID, SERVER_ID, PacketDef::Flag::BB, 1, 1);
                blackbox_data.setData(posBuff, positionLength);
                blackbox_data.setCrc(0);
                if (blackbox_data.getData() == nullptr)
                {
                    std::cout << "Error setting data: size too large or error allocating memory." << std::endl;
                }

                // Serialize the packet
                unsigned int totalSize = blackbox_data.Serialize(buffer);
                if (buffer != nullptr) {
					std::cout << "Preparing to send..." << std::endl;

                    // Send the packet
					sendto(connectionDetails.socket, buffer, totalSize, 0, (struct sockaddr*)&rxSender, sizeof(rxSender));
					std::cout << "Sent black box data." << std::endl;

                    // Receive a response
					bytesRead = recvfrom(connectionDetails.socket, recvBuffer, Constants::MAX_PACKET_LENGTH, 0, (struct sockaddr*)&rxSender, &addrLength);
					std::cout << "Received response." << std::endl;
					received = PacketDef(recvBuffer, bytesRead);

					// Check for ACK
					if (received.getFlag() != PacketDef::Flag::ACK)
					{
						std::cerr << "Error: No ACK received." << std::endl;
					}

                }

                
			}
            else if (!send_blackbox_data) {   // Request an image

            }
			else {  // Error handling

            }

			Sleep(1000);	// Sleep for 1 second
		}
    }

    return 1;
}