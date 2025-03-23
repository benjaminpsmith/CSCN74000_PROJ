// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "image.h"
#include <thread>

// using namespace ;
using namespace WeatherImage;

int main(void) {

    ConnectionData::ConnDetails connectionDetails;
    ConnectionData::Connection flightConnection;
    PacketData::PacketDef received;
    PacketData::PacketDef toSend;
    bool shutdown;
    char recvBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
    ConnectionData::address rxSender;
    int addrLength;
    int bytesRead;
    PacketData::PacketDef beginsHandshake;
    int err;

    shutdown = false;
    bytesRead = 0;
    addrLength = 0;
    err = 0;

	if (memset(&connectionDetails.addr, 0, sizeof(connectionDetails.addr)) != &connectionDetails.addr)
	{
		std::cerr << "Error initializing connection details." << std::endl;
	}
	if (memset(&rxSender, 0, sizeof(rxSender)) != &rxSender)
	{
		std::cerr << "Error initializing rxSender." << std::endl;
	}

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_PORT, SERVER_IP);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    addrLength = sizeof(rxSender);

    if (beginsHandshake.setData(SECURE_PASSWORD, SECURE_PASSWORD_LEN) == -1) {
		std::cerr << "Error setting data: size too large or error allocating memory." << std::endl;
    }
    beginsHandshake.setDest(SERVER_ID);
    beginsHandshake.setSeqNum(0);
    beginsHandshake.setSrc(AIRPLANE_ID);
    beginsHandshake.setTotalCount(1);

    while (!shutdown)
    { 
        while (flightConnection.getAuthenticationState() != ConnectionData::ConnState::AUTHENTICATED)
        {
            if (flightConnection.getAuthenticationState() == ConnectionData::ConnState::UNAUTHENTICATED)
            {
                //start the connection handshake
                int retValue = flightConnection.establishConnection(beginsHandshake, &connectionDetails.addr);
                if (retValue != 1)
                {
					std::cerr << "Error establishing connection." << std::endl;
                }
            }
            else
            {
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);

                err = WSAGetLastError();

                if (bytesRead > 0)
                {
                    received = PacketData::PacketDef(recvBuffer, bytesRead);
                    int connectionSuccess = flightConnection.establishConnection(received, &rxSender);
                    if (connectionSuccess != 1)
                    {
						std::cerr << "Error establishing connection." << std::endl;
                    }
                }
            }
            
            if (flightConnection.getAuthenticationState() != ConnectionData::ConnState::AUTHENTICATED)
            {
                Sleep(1);
            }
        }

        // Client has now successfully been authenticated.
        // The client will now alternate between sending black box data to the server and requests for images.
		std::cout << " Client has been authenticated." << std::endl;
		std::cout << " Client will now alternate between sending black box data to the server and requests for images.\n" << std::endl;

		while (flightConnection.getAuthenticationState() == ConnectionData::ConnState::AUTHENTICATED)   // Loop
		{
            bool send_blackbox_data = false;

			if (send_blackbox_data) { // Send black box data

                // Create buffers to hold the serialized position data and the serialized packet
                char posBuff[PacketData::Constants::MAX_PACKET_LENGTH] = { 0 };
                char buffer [PacketData::Constants::MAX_PACKET_LENGTH] = { 0 };
                
                // Generate a current position (currently random for proof of concept)
                PositionData::Position currentPosition;
                currentPosition.createRandomValues();
                int positionLength = currentPosition.Serialize(posBuff);
				std::cout << currentPosition.latitude << " " << currentPosition.longitude << " " << currentPosition.heading << " " << currentPosition.velocity << " " << currentPosition.altitude << std::endl;

                // Construct the packet to send
                PacketData::PacketDef blackbox_data(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::BB, 1, 1);
                if (blackbox_data.setData(posBuff, positionLength) == -1) {
					std::cerr << "Error setting data: size too large or error allocating memory." << std::endl;
                }
                blackbox_data.setCrc(0);
                if (blackbox_data.getData() == nullptr)
                {
                    std::cout << "Error setting data: size too large or error allocating memory." << std::endl;
                }

                // Serialize the packet
                if (buffer != nullptr) {
                    unsigned int totalSize = blackbox_data.Serialize(buffer);
					std::cout << "Preparing to send..." << std::endl;

                    // Send the packet
					int sendToRetVale = sendto(connectionDetails.socket, buffer, totalSize, 0, reinterpret_cast<struct sockaddr*>(&rxSender), sizeof(rxSender));
					std::cout << "Sent black box data." << std::endl;

                    // Receive a response
					bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
					std::cout << "Received response." << std::endl;
					received = PacketData::PacketDef(recvBuffer, bytesRead);

					// Check for ACK
					if (received.getFlag() != PacketData::PacketDef::Flag::ACK)
					{
						std::cerr << "Error: No ACK received." << std::endl;
					}
                } 
			}
            else if (!send_blackbox_data) {   // Request an image

                char buffer[PacketData::Constants::MAX_PACKET_LENGTH] = { 0 };

                // Construct the packet to send
                PacketData::PacketDef imgRequest(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::IMG, 1, 1);
                PacketData::PacketDef ackReceived(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::ACK, 1, 1);
                imgRequest.setCrc(0);

                if (buffer)
                {
                    Image imgReceived;
                    int flag = 0;
                    int packetsToReceive = 0;
                    unsigned int totalSize = imgRequest.Serialize(buffer);
                    std::cout << "Preparing to send..." << std::endl;

                    //Request
                    int sendToRetVal = sendto(connectionDetails.socket, buffer, totalSize, 0, reinterpret_cast<struct sockaddr*>(&rxSender), sizeof(rxSender));
                    std::cout << "Sent image request." << std::endl;

                    //Get Response ACK for Request
                    bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                    received = PacketDef(recvBuffer, bytesRead);

                    flag = received.getFlag();
                    if (flag == PacketDef::Flag::SHUTDOWN)
                    {
                        shutdown = true;
                        break;
                    }
                    else if (flag == PacketDef::ACK)
                    {
                        packetsToReceive = received.getTotalCount();

                        //Receive incoming Image packets
                        while (received.getSeqNum() != packetsToReceive)
                        {
                            
                            bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);

                            received = PacketDef(recvBuffer, bytesRead);
                            imgReceived.addSome(received);

                            int bytesToSend = ackReceived.Serialize(buffer);  // SEND ACK
                            int sendResult = sendto(connectionDetails.socket, buffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                        }

                        std::cout << "Received " << packetsToReceive << " Image packets" << std::endl;

                        //write image to file
                        if (imgReceived.saveImage())
                        {
                            std::cout << "Successfully saved the transmitted image" << std::endl;
                        }
                    }
                }
            }
			else {  // Error handling

            }

			Sleep(1000);	// Sleep for 1 second
		}
    }

    return 1;
}