// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "image.h"
#include <thread>


#define BLACKBOX_FILE "_blackbox.csv"

namespace Server {

    using namespace PositionData;
    using namespace PacketData;
    using namespace ConnectionData;
    using namespace WeatherImage;

    typedef enum SERVER_STATE : int8_t
    {
        AWAITING_AUTH,
        IDLE,
        PROCESSING,
        SENDING,
        RECEIVING
    }ServerState;

    int serverThread(PacketDef&, bool, int);

}

int main(void){
    PacketData::PacketDef received;
    bool firstHandshakePacket = false;

    int result = Server::serverThread(received, firstHandshakePacket, SERVER_PORT);
}

int Server::serverThread(PacketDef& received, bool firstHandshakePacket, int serverPort)
{
    ServerState state;
    ConnDetails connectionDetails;
    Connection flightConnection;
    PacketDef toSend;
    bool shutdown;
    char recvBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
    address rxSender;
    int addrLength;
    int bytesRead;
    std::thread timerThread;
    bool secondElapsed;
    int err;
    int bindToRetVal;


    bindToRetVal = 0;
    state = AWAITING_AUTH;
    secondElapsed = false;
    shutdown = false;
    bytesRead = 0;
    err = 0;


    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket(); 
    connectionDetails.addr = flightConnection.createAddress(serverPort, SERVER_IP);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 500;


    bindToRetVal = flightConnection.bindTo(&connectionDetails.socket, &connectionDetails.addr);

    addrLength = sizeof(rxSender);

    //server states:
    //awaiting, idle, processing, sending or receiving


    while (!shutdown)
    {
        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            state = AWAITING_AUTH;
            std::cout << "Server awaiting authentication..." << std::endl;

            //Only the main thread would not have a packet provided to it. Other threads will have an auth bit set in a packet passed to it as a result of the parent thread receiving a packet with an auth bit.
            //The packet is passed to the new thread, and that new thread will use that packet as the starting point for the handshake
            if (!firstHandshakePacket)
            {
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);

                err = WSAGetLastError();

                received = PacketDef(recvBuffer, bytesRead);
            }

            firstHandshakePacket = false;
            int retValue = flightConnection.accept(received, &rxSender);
			if (retValue != 1)
			{
				std::cerr << "Error accepting connection." << std::endl;
			}

            if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
            {
                Sleep(1);
                std::cout << "Sleeping for one second..." << std::endl;
            }
        }

        //initially this is 0, if nothing is received it is -1, and only if we receive do we leave the idle state. We go back to the idle state after 1 instance of receiving nothing.
        if (bytesRead == 0)
        {
            state = ServerState::IDLE;
        }

        std::cout << "Server is now idle and waiting to receive packets..." << std::endl;
        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
        if (bytesRead == -1) {
            std::cerr << "Error in recvfrom(): " << WSAGetLastError() << std::endl;
        }
        else {
            std::cout << "Server received " << bytesRead << " bytes." << std::endl;
        }
        received = PacketDef(recvBuffer, bytesRead);    //RECV BB DATA
        PacketDef::Flag incomingFlag = received.getFlag();

        // Check if it is a black-box packet or a request for an image
        switch (incomingFlag) {
        case PacketDef::Flag::BB:   
        {
			// We are now receiving the black-box data from the client
            state = ServerState::RECEIVING;

            std::cout << "Server is now receiving" << std::endl;

			const char* data = received.getData();
            Position pos(data);
            std::cout << pos.latitude << " " << pos.longitude << " " << pos.heading << " " << pos.velocity << " " << pos.altitude << std::endl;

                                          // Create a position object from the string     
            std::string filename = std::to_string(received.getSrc()) + BLACKBOX_FILE;    // Create the filename for the black-box data
            bool retValue = pos.writeToFile(filename);	                                            // Write the black-box data to a file named "clientID_blackbox.csv"
            if (retValue) {
                std::cout << "Black-box data received and written to file." << std::endl;
            }
            else {
				std::cerr << "Error writing black-box data to file." << std::endl;
            }

            // Send an ACK back to the client -  could potentially make one ahead of time and use it repeatedly since the body is empty, would only need to change dest?
            toSend.setSrc(SERVER_ID);
            toSend.setDest(received.getSrc());
            toSend.setFlag(PacketDef::Flag::ACK);
            toSend.setSeqNum(1);
            toSend.setTotalCount(1);
            toSend.setBodyLen(0);
			if (toSend.setData(nullptr, 0) == -1) {
				std::cerr << "Error setting data: size too large or error allocating memory." << std::endl;
			}
            toSend.setCrc(0);
            char sendBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
            int bytesToSend = toSend.Serialize(sendBuffer);  // SEND ACK
            int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
            break;
        }
        case PacketDef::Flag::IMG:
        {
            state = ServerState::RECEIVING;
            std::cout << "Server is now receiving" << std::endl;

            std::cout << "Server received " << bytesRead << " bytes." << std::endl;
            PacketData::PacketDef ackReceived(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::ACK, 1, 1);
            PacketData::PacketDef nackResponse(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::EMPTY, 1, 1);
            PacketData::PacketDef shutdownResponse(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::SHUTDOWN, 1, 1);
            int i = 0;
            int attempts = 0;
            int packetCount = 0;
            const std::vector<PacketDef*>* packetList = nullptr;
            Image weatherImage;

            state = ServerState::PROCESSING;
            std::cout << "Server is now processing" << std::endl;

            weatherImage.loadImage();
            packetList = weatherImage.getPacketList();

            char sendBuffer[PacketData::Constants::MAX_PACKET_LENGTH];

            struct timeval read_timeout;
            read_timeout.tv_sec = 0;
            read_timeout.tv_usec = 10;

            //ACK for the request
            ackReceived.setTotalCount(weatherImage.getPacketCount());
            int bytesToSend = ackReceived.Serialize(sendBuffer);  // SEND ACK
            int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);

            //setting a socket timeout
            /*
            err = setsockopt(connectionDetails.socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            if (err == SOCKET_ERROR) {
                std::cerr << "Failed to configure socket timeout" << std::endl;
                int bytesToSend = nackResponse.Serialize(sendBuffer);  // SEND NACK
                int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                break;
            }
            else
                std::cout << "Configured socket timeout" << std::endl;

            */
            
            state = ServerState::SENDING;
            std::cout << "Server is now sending" << std::endl;

            std::cout << "Image packets to send: " << packetList->size() << std::endl;

            while (i < packetList->size())
            {
                PacketDef* imagePacket = packetList->at(i);
                bytesRead = 0;

                int bytesToSend = imagePacket->Serialize(sendBuffer);  // Send image packet after packet, and expect ACK responses for each
                int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);

                if (sendResult > 0)
                {
                    bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);

                    if (bytesRead < 0)
                    {
                        //timeout
                        attempts++;
                    }
                    else
                    {
                        //success
                        received = PacketDef(recvBuffer, bytesRead);
                        if (received.getFlag() == PacketDef::Flag::ACK)
                        {
                            i++;
                        }
                    }
                }
                
                if (attempts >= 3)
                {
                    std::cerr << "Failed to send image: 3 missed ACKS" << std::endl;
                    break;
                }
            }

            read_timeout.tv_sec = 0;
            read_timeout.tv_usec = 0;

            /*
            //return socket to blocking - no timeout
            err = setsockopt(connectionDetails.socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
            if (err == SOCKET_ERROR) {
                std::cerr << "Failed to configure socket timeout" << std::endl;
                int bytesToSend = shutdownResponse.Serialize(sendBuffer);  // SEND shutdown
                int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                break;
            }
            else
                std::cout << "Configured socket timeout" << std::endl;
            
            */
            
            break;
        }
    }
    
        if (bytesRead <= 0)
        {
            state = ServerState::IDLE;
            Sleep(1);
        }

    }

    //This code has been added to simply keep the console window open until you type a character.
    int garbage;
    std::cin >> garbage;

    return 1;
}
