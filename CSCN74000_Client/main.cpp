// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "image.h"
#include <thread>
#include "menu.h"

#define RECV_MSG_MAX_LEN 12
#define SEND_MSG_MAX_LEN 10

// using namespace ;

int main(void) {

    ConnectionData::ConnDetails connectionDetails;
    ConnectionData::Connection flightConnection;
    PacketData::PacketDef received;
    PacketData::PacketDef toSend;
    ui::Menu log;
    bool shutdown;
    char recvBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
    ConnectionData::address rxSender;
    int addrLength;
    int bytesRead;
    PacketData::PacketDef beginsHandshake;
    int err;
    char sendingMsg[10] = "Sending: ";
    char receivingMsg[13] = "Receiving: ";
    char clsCmd[4] = "cls";
    const uint16_t PORT = SERVER_PORT;
    const char* IPADDRESS = SERVER_IP;
    const char* SECUREPASSWD = SECURE_PASSWORD;
    const int SERVERID = SERVER_ID;
    const int AIRPLANEID = AIRPLANE_ID;
    shutdown = false;
    bytesRead = 0;
    addrLength = 0;
    err = 0;
    bool innerContinue = true;
    bool imageLoopContinue = true;

	if (memset(&connectionDetails.addr, 0, sizeof(connectionDetails.addr)) != &connectionDetails.addr)
	{

        log << "Error initializing connection details.";
	}
	if (memset(&rxSender, 0, sizeof(rxSender)) != &rxSender)
	{
        log << "Error initializing rxSender.";
	}

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket();
    connectionDetails.addr = flightConnection.createAddress(PORT, IPADDRESS);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECUREPASSWD);

    addrLength = sizeof(rxSender);

    if (beginsHandshake.setData(SECUREPASSWD, SECURE_PASSWORD_LEN) == -1) {
        log << "Error setting data: size too large or error allocating memory.";
    }
    beginsHandshake.setDest(SERVERID);
    beginsHandshake.setSeqNum(0);
    beginsHandshake.setSrc(AIRPLANEID);
    beginsHandshake.setTotalCount(1);

    while (!shutdown)
    { 
        if (std::system(&clsCmd[0]) != 0)
        {
            log << "Failed to clear the screen";
        }
        log.printLog();

        while (flightConnection.getAuthenticationState() != ConnectionData::ConnState::AUTHENTICATED)
        {
            log.printLog();

            if (flightConnection.getAuthenticationState() == ConnectionData::ConnState::UNAUTHENTICATED)
            {
                //start the connection handshake
                int retValue = flightConnection.establishConnection(beginsHandshake, &connectionDetails.addr);
                if (retValue <= 0)
                {
                    log << "Error establishing connection.";
                }
            }
            else
            {
                bytesRead = recvfrom(connectionDetails.socket, &recvBuffer[0], PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                if (!log.writeToFileLog(&receivingMsg[0], RECV_MSG_MAX_LEN))
                {
                    log << "Logger failed to write to file";
                }
                if (!log.writeToFileLog(&recvBuffer[0], bytesRead))
                {
                    log << "Logger failed to write to file";
                }
                
                err = WSAGetLastError();

                if (bytesRead > 0)
                {
                    received = PacketData::PacketDef(&recvBuffer[0], bytesRead);
                    log << received;
                    int connectionSuccess = flightConnection.establishConnection(received, &rxSender);
                    if (connectionSuccess != 1)
                    {
						log << "Error establishing connection.";
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
		log << "Client has been authenticated.\n";
        log << "Client will now alternate between sending black box data to the server and requests for images.\n";

        innerContinue = true;

        bool send_blackbox_data = false;

		while (flightConnection.getAuthenticationState() == ConnectionData::ConnState::AUTHENTICATED && !shutdown && innerContinue)   // Loop
		{
            

            log.printLog();

			if (send_blackbox_data) { // Send black box data

                // Create buffers to hold the serialized position data and the serialized packet
                char posBuff[PacketData::Constants::MAX_PACKET_LENGTH] = { 0 };
                char buffer [PacketData::Constants::MAX_PACKET_LENGTH] = { 0 };
                
                // Generate a current position (currently random for proof of concept)
                PositionData::Position currentPosition;
                currentPosition.createRandomValues();
                int positionLength = currentPosition.Serialize(&posBuff[0]);
				std::cout << currentPosition.latitude << " " << currentPosition.longitude << " " << currentPosition.heading << " " << currentPosition.velocity << " " << currentPosition.altitude << std::endl;

                // Construct the packet to send
                PacketData::PacketDef blackbox_data(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::BB, 1, 1);
                if (blackbox_data.setData(&posBuff[0], positionLength) == -1) {
					log << "Error setting data: size too large or error allocating memory.";
                }
                blackbox_data.setCrc(0);
                if (blackbox_data.getData() == nullptr)
                {
                    log << "Error setting data: size too large or error allocating memory.";
                }

                // Serialize the packet
                if (buffer != nullptr) {
                    unsigned int totalSize = blackbox_data.Serialize(&buffer[0]);

                    // Send the packet
					int sendToRetVale = sendto(connectionDetails.socket, &buffer[0], totalSize, 0, reinterpret_cast<struct sockaddr*>(&rxSender), sizeof(rxSender));
					log << "Sent black box data.";
                    if (!log.writeToFileLog(&sendingMsg[0], SEND_MSG_MAX_LEN))
                    {
                        log << "Logger failed to write to file";
                    }
                    if (!log.writeToFileLog(&buffer[0], totalSize))
                    {
                        log << "Logger failed to write to file";
                    }

                    // Receive a response
					bytesRead = recvfrom(connectionDetails.socket, &recvBuffer[0], PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                    if (!log.writeToFileLog(&receivingMsg[0], RECV_MSG_MAX_LEN))
                    {
                        log << "Logger failed to write to file";
                    }
                    if (!log.writeToFileLog(&recvBuffer[0], bytesRead))
                    {
                        log << "Logger failed to write to file";
                    }

					received = PacketData::PacketDef(&recvBuffer[0], bytesRead);
                    log << received;
					// Check for ACK
					if (received.getFlag() != PacketData::PacketDef::Flag::ACK)
					{
						log << "Error: No ACK received.";
                        
					}
                    if (received.getFlag() == PacketData::PacketDef::Flag::SHUTDOWN)
                    {
                        log << "Received shutdown message.";
                        shutdown = true;
                        innerContinue = false;
                    }
                    if (received.getFlag() == PacketData::PacketDef::AUTH_LOST)
                    {
                        flightConnection.restartAuth();
                        innerContinue = false;
                    }
                } 

                send_blackbox_data = false;
			}
            else if (!send_blackbox_data) {   // Request an image

                char buffer[PacketData::Constants::MAX_PACKET_LENGTH] = { 0 };

                // Construct the packet to send
                PacketData::PacketDef imgRequest(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::IMG, 1, 1);
                PacketData::PacketDef ackReceived(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::ACK, 1, 1);
                imgRequest.setCrc(0);

                if (buffer)
                {
                    WeatherImage::Image imgReceived;
                    int flag = 0;
                    unsigned int totalSize = imgRequest.Serialize(&buffer[0]);
                    log << "Preparing to send";

                    //Request
                    int sendToRetVal = sendto(connectionDetails.socket, &buffer[0], totalSize, 0, reinterpret_cast<struct sockaddr*>(&rxSender), sizeof(rxSender));
                    log << "Sent image request";
                    if (!log.writeToFileLog(&sendingMsg[0], RECV_MSG_MAX_LEN))
                    {
                        log << "Logger failed to write to file";
                    }
                    if (!log.writeToFileLog(&buffer[0], totalSize))
                    {
                        log << "Logger failed to write to file";
                    }

                    //Get Response ACK for Request
                    bytesRead = recvfrom(connectionDetails.socket, &recvBuffer[0], PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                    if (!log.writeToFileLog(&receivingMsg[0], RECV_MSG_MAX_LEN))
                    {
                        log << "Logger failed to write to file";
                    }
                    if(!log.writeToFileLog(&recvBuffer[0], bytesRead))
                    {
                        log << "Logger failed to write to file";
                    }

                    if (received.getFlag() == PacketData::PacketDef::AUTH_LOST)
                    {
                        flightConnection.restartAuth();
                    }
                    received = PacketData::PacketDef(&recvBuffer[0], bytesRead);
                    log << received;

                    flag = received.getFlag();
                    if (flag == PacketData::PacketDef::Flag::SHUTDOWN)
                    {
                        shutdown = true;
                        innerContinue = false;
                    }
                    else if (flag == PacketData::PacketDef::ACK)
                    {

                        int packetsToReceive = 0;
                        const int LOGMSGFORMAT = 9; 
                        const size_t LOGMSGLEN = 40;
                        char logBuffer[LOGMSGLEN] = { 0 };
                        char logFormat[LOGMSGFORMAT] = "%s %d %s";

                        packetsToReceive = received.getTotalCount();

                        //Receive incoming Image packets

                        imageLoopContinue = true;

                        log << "Receiving image packets";

                        log.printLog();

                        while (received.getSeqNum() != packetsToReceive && imageLoopContinue == true)
                        {

                            bytesRead = recvfrom(connectionDetails.socket, &recvBuffer[0], PacketData::Constants::MAX_PACKET_LENGTH, 0, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                            if(!log.writeToFileLog(&receivingMsg[0], RECV_MSG_MAX_LEN))
                            {
                                log << "Logger failed to write to file";
                            }
                            if(!log.writeToFileLog(&recvBuffer[0], bytesRead))
                            {
                                log << "Logger failed to write to file";
                            }

                            if (received.getFlag() == PacketData::PacketDef::AUTH_LOST)
                            {
                                flightConnection.restartAuth();
                                imageLoopContinue = false;
                            }
                            received = PacketData::PacketDef(&recvBuffer[0], bytesRead);
                            log << received;
                            imgReceived.addSome(received);

                            int bytesToSend = ackReceived.Serialize(&buffer[0]);  // SEND ACK
                            int sendResult = sendto(connectionDetails.socket, &buffer[0], bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                            if(!log.writeToFileLog(&sendingMsg[0], static_cast<int>(strnlen(&receivingMsg[0], RECV_MSG_MAX_LEN))))
                            {
                                log << "Loggesr failed to write to file";
                            }
                            if(!log.writeToFileLog(&buffer[0], bytesToSend))
                            {
                                log << "Logger failed to write to file";
                            }
                        }
                        
                        err = sprintf_s(&logBuffer[0], LOGMSGLEN, &logFormat[0], &receivingMsg[0], packetsToReceive, "image packets");
                        log << logBuffer;

                        //write image to file - 
                        if (imgReceived.saveImage())
                        {
                            log << "Successfully saved the transmitted image";
                        }
                    }
                    else
                    {
                        log << "A different type of packet was received. Where did you come from?";
                    }
                }

                send_blackbox_data = true;
            }
			else {  // Error handling

            }

			Sleep(1000);	// Sleep for 1 second
		}
    }
    log << "Shutting down";
    return 1;
}