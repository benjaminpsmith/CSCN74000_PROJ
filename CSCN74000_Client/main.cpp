// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "image.h"
#include <thread>
#include "menu.h"

// using namespace ;
using namespace WeatherImage;

int main(void) {

    ConnectionData::ConnDetails connectionDetails;
    ConnectionData::Connection flightConnection;
    PacketData::PacketDef received;
    PacketData::PacketDef toSend;
    Menu log;
    bool shutdown;
    char recvBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
    ConnectionData::address rxSender;
    int addrLength;
    int bytesRead;
    PacketData::PacketDef beginsHandshake;
    int err;
    char sendingMsg[10] = "Sending: ";
    char receivingMsg[13] = "Receiving: ";

    shutdown = false;
    bytesRead = 0;
    addrLength = 0;
    err = 0;

	if (memset(&connectionDetails.addr, 0, sizeof(connectionDetails.addr)) != &connectionDetails.addr)
	{

        log << "Error initializing connection details.\n";
	}
	if (memset(&rxSender, 0, sizeof(rxSender)) != &rxSender)
	{
        log << "Error initializing rxSender.\n";
	}

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(SERVER_PORT, SERVER_IP);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    addrLength = sizeof(rxSender);

    if (beginsHandshake.setData(SECURE_PASSWORD, SECURE_PASSWORD_LEN) == -1) {
        log << "Error setting data: size too large or error allocating memory.\n";
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
                    log << "Error establishing connection.\n";
                }
            }
            else
            {
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                log.writeToFileLog(receivingMsg, strlen(receivingMsg));
                log.writeToFileLog(recvBuffer, bytesRead);
                err = WSAGetLastError();

                if (bytesRead > 0)
                {
                    received = PacketData::PacketDef(recvBuffer, bytesRead);
                    int connectionSuccess = flightConnection.establishConnection(received, &rxSender);
                    if (connectionSuccess != 1)
                    {
						log << "Error establishing connection.\n";
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
		log << " Client has been authenticated.\n";
        log << " Client will now alternate between sending black box data to the server and requests for images.\n";

		while (flightConnection.getAuthenticationState() == ConnectionData::ConnState::AUTHENTICATED && !shutdown)   // Loop
		{
            bool send_blackbox_data = true;

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
					log << "Error setting data: size too large or error allocating memory.\n";
                }
                blackbox_data.setCrc(0);
                if (blackbox_data.getData() == nullptr)
                {
                    log << "Error setting data: size too large or error allocating memory.\n";
                }

                // Serialize the packet
                if (buffer != nullptr) {
                    unsigned int totalSize = blackbox_data.Serialize(buffer);

                    // Send the packet
					int sendToRetVale = sendto(connectionDetails.socket, buffer, totalSize, 0, reinterpret_cast<struct sockaddr*>(&rxSender), sizeof(rxSender));
					log << "Sent black box data.\n";
                    log.writeToFileLog(sendingMsg, strlen(receivingMsg));
                    log.writeToFileLog(buffer, totalSize);

                    // Receive a response
					bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                    log.writeToFileLog(receivingMsg, strlen(receivingMsg));
                    log.writeToFileLog(recvBuffer, bytesRead);

					received = PacketData::PacketDef(recvBuffer, bytesRead);

					// Check for ACK
					if (received.getFlag() != PacketData::PacketDef::Flag::ACK)
					{
						log << "Error: No ACK received.\n";
                        
					}
                    if (received.getFlag() == PacketData::PacketDef::Flag::SHUTDOWN)
                    {
                        log << "Received shutdown message.\n";
                        shutdown = true;
                        break;
                    }
                    if (received.getFlag() == PacketData::PacketDef::AUTH_LOST)
                    {
                        flightConnection.restartAuth();
                        break;
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
                    log << "Preparing to send...\n";

                    //Request
                    int sendToRetVal = sendto(connectionDetails.socket, buffer, totalSize, 0, reinterpret_cast<struct sockaddr*>(&rxSender), sizeof(rxSender));
                    log << "Sent image request.\n";
                    log.writeToFileLog(sendingMsg, strlen(receivingMsg));
                    log.writeToFileLog(buffer, totalSize);

                    //Get Response ACK for Request
                    bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                    log.writeToFileLog(receivingMsg, strlen(receivingMsg));
                    log.writeToFileLog(recvBuffer, bytesRead);

                    if (received.getFlag() == PacketData::PacketDef::AUTH_LOST)
                    {
                        flightConnection.restartAuth();
                    }
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
                            log.writeToFileLog(receivingMsg, strlen(receivingMsg));
                            log.writeToFileLog(recvBuffer, bytesRead);

                            if (received.getFlag() == PacketData::PacketDef::AUTH_LOST)
                            {
                                flightConnection.restartAuth();
                                break;
                            }
                            received = PacketDef(recvBuffer, bytesRead);
                            imgReceived.addSome(received);

                            int bytesToSend = ackReceived.Serialize(buffer);  // SEND ACK
                            int sendResult = sendto(connectionDetails.socket, buffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                            log.writeToFileLog(sendingMsg, strlen(receivingMsg));
                            log.writeToFileLog(buffer, totalSize);
                        }

                        log << (std::string("Received") + std::to_string(packetsToReceive) + std::string(" Image packets\n")).c_str();

                        //write image to file - 
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