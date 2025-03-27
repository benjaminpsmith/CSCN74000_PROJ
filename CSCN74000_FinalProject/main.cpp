// Source file for main of the server
#include "packet.h"
#include "connection.h"
#include "image.h"
#include <thread>
#include "menu.h"
#include <atomic>
using namespace ConnectionData;


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

    int serverThread(PacketDef& received, bool firstHandshakePacket, int serverPort, bool& shutdown, Menu& menu);
    int menuThread(Menu& menu, bool& shutdown);

}

int main(void){

    PacketData::PacketDef received;
    bool firstHandshakePacket = false;
    bool shutdown = false;
    bool closeMenu = false;
    Menu mainMenu;



    std::thread menuThread([&] {
        int result = Server::menuThread(mainMenu, closeMenu);
        });

    std::thread serverThread([&] {
        int result = Server::serverThread(received, firstHandshakePacket, SERVER_PORT, shutdown, mainMenu);
        });

    while (!shutdown)
    {
        char opt = 0;
        //clear the console
        std::system("cls");
        mainMenu.printMenu();

        opt = getchar();
        
        switch (opt)
        {
        case '1':
        {
            shutdown = true;
            break;
        }
        default:
            break;
        }

        Sleep(500);
    }
    
    serverThread.join();
    closeMenu = true;
    menuThread.join();
    
}

int Server::serverThread(PacketDef& received, bool firstHandshakePacket, int serverPort, bool& shutdown, Menu& log)
{
    Server::ServerState state;
    ConnDetails connectionDetails;
    Connection flightConnection;
    PacketDef toSend;
    PacketData::PacketDef ackReceived(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::ACK, 1, 1);
    PacketData::PacketDef shutdownResponse(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::SHUTDOWN, 1, 1);
    PacketData::PacketDef reauthResponse(AIRPLANE_ID, SERVER_ID, PacketData::PacketDef::Flag::AUTH_LOST, 1, 1);
    char sendBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
    char recvBuffer[PacketData::Constants::MAX_PACKET_LENGTH];
    char sendingMsg[10] = "Sending: ";
    char receivingMsg[13] = "Receiving: ";
    char messageBuff[256] = { 0 };

    int attempts = 0;
    int packetCount = 0;

    address rxSender;
    int addrLength;
    int bytesRead;
    std::thread timerThread;
    bool thirtySecondElapsed;
    std::atomic<bool> watchDogKick;
    int err;
    int bindToRetVal;
    PacketDef::Flag incomingFlag;
    bool watchdogError;

    watchdogError = false;
    watchDogKick = true;
    bindToRetVal = 0;
    state = Server::SERVER_STATE::AWAITING_AUTH;
    thirtySecondElapsed = false;
    shutdown = false;
    bytesRead = 0;
    err = 0;
    incomingFlag = PacketDef::Flag::EMPTY;

    //set the connection details and creat the socket
    connectionDetails.socket = flightConnection.createSocket(); 
    connectionDetails.addr = flightConnection.createAddress(serverPort, SERVER_IP);
    flightConnection.setConnectionDetails(&connectionDetails.socket, &connectionDetails.addr);
    flightConnection.setPassphrase(SECURE_PASSWORD);

    bindToRetVal = flightConnection.bindTo(&connectionDetails.socket, &connectionDetails.addr);

    addrLength = sizeof(rxSender);

    //server states:
    //awaiting, idle, processing, sending or receiving

    timerThread = std::thread([&] {
            while (!shutdown)
            {
                Sleep(60000);
                if (watchDogKick == false)
                {
                    watchdogError = true;
                }
                else
                {
                    watchDogKick = false;
                }
            }
        });


    while (!shutdown && !watchdogError)
    {
        incomingFlag = PacketDef::Flag::EMPTY;

        while (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
        {
            state = Server::SERVER_STATE::AWAITING_AUTH;
            log << "Server awaiting authentication...";

            //Only the main thread would not have a packet provided to it. Other threads will have an auth bit set in a packet passed to it as a result of the parent thread receiving a packet with an auth bit.
            //The packet is passed to the new thread, and that new thread will use that packet as the starting point for the handshake
            if (!firstHandshakePacket)
            {
                bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                log.writeToFileLog(receivingMsg, strlen(receivingMsg));
                log.writeToFileLog(recvBuffer, bytesRead);
                if (bytesRead > 0)
                {
                    err = WSAGetLastError();
                    received = PacketDef(recvBuffer, bytesRead);
                }
            }

            firstHandshakePacket = false;

            if (bytesRead > 0)
            {
                int retValue = flightConnection.accept(received, &rxSender);
                if (retValue != 1)
                {
                    log << "Error accepting connection.";
                }

                if (flightConnection.getAuthenticationState() != ConnState::AUTHENTICATED)
                {
                    Sleep(1);
                    log << "Sleeping for one second...";
                }
            }
            else
            {
                flightConnection.restartAuth();
            }
            
        }

        //initially this is 0, if nothing is received it is -1, and only if we receive do we leave the idle state. We go back to the idle state after 1 instance of receiving nothing.
        if (bytesRead == 0)
        {
            //setting the idle state
            state = Server::SERVER_STATE::IDLE;
        }

        log << "Server is now idle and waiting to receive packets...";
        bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
        log.writeToFileLog(receivingMsg, strlen(receivingMsg));
        log.writeToFileLog(recvBuffer, bytesRead);

        if (bytesRead < 0) {
            
                log << "Error in recvfrom() or timed out";
                flightConnection.restartAuth();
                int bytesToSend = reauthResponse.Serialize(sendBuffer);  // SEND REAUTH
                int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                incomingFlag = PacketDef::Flag::EMPTY;
        }
        else {
            //kick the waatchdog so that the watchdog doesnt set a boolean to shutdown the server
            watchDogKick = true;
            log << "Server received a command from client";
            // We are now receiving the black-box data from the client
            state = Server::SERVER_STATE::RECEIVING;
            log << "Server is now receiving";
            received = PacketDef(recvBuffer, bytesRead);    //RECV BB DATA
            incomingFlag = received.getFlag();
        }

        try {
            // Check if it is a black-box packet or a request for an image
            switch (incomingFlag) {
                case PacketDef::Flag::BB://BB data received by server
                {
                    const char* data = received.getData();
                    PositionData::Position pos(data);
                    sprintf(messageBuff, "%.3f, %.3f, %.3f, %.3f, %.3f", pos.latitude, pos.longitude, pos.heading, pos.velocity, pos.altitude);
                    log << messageBuff;
                    // Create a position object from the string     
                    std::string filename = std::to_string(received.getSrc()) + BLACKBOX_FILE;    // Create the filename for the black-box data
                    bool retValue = pos.writeToFile(filename);	                                            // Write the black-box data to a file named "clientID_blackbox.csv"
                    if (retValue) {
                        log << "Black-box data received and written to file.";
                    }
                    else {
                        log << "Error writing black-box data to file.";
                    }

                    // Send an ACK back to the client -  could potentially make one ahead of time and use it repeatedly since the body is empty, would only need to change dest?
                    toSend.setSrc(SERVER_ID);
                    toSend.setDest(received.getSrc());
                    toSend.setFlag(PacketDef::Flag::ACK);
                    toSend.setSeqNum(1);
                    toSend.setTotalCount(1);
                    toSend.setBodyLen(0);
                    if (toSend.setData(nullptr, 0) == -1) {
                        log << "Error setting data: size too large or error allocating memory.";
                    }
                    toSend.setCrc(0);

                    int bytesToSend = toSend.Serialize(sendBuffer);  // SEND ACK
                    int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                    log.writeToFileLog(sendingMsg, strlen(receivingMsg));
                    log.writeToFileLog(sendBuffer, bytesToSend);
                    break;
                }
                case PacketDef::Flag::IMG://Image request received by server
                {

                    int i = 0;

                    const std::vector<PacketDef*>* packetList = nullptr;
                    WeatherImage::Image weatherImage;

                    state = Server::SERVER_STATE::PROCESSING;
                    log << "Server is now processing";

                    weatherImage.loadImage();
                    packetList = weatherImage.getPacketList();

                    //ACK for the request
                    ackReceived.setTotalCount(weatherImage.getPacketCount());
                    int bytesToSend = ackReceived.Serialize(sendBuffer);  // SEND ACK
                    int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                    log.writeToFileLog(sendingMsg, strlen(receivingMsg));
                    log.writeToFileLog(sendBuffer, bytesToSend);

                    //Image request handling is the only case where the server enters the processing state.
                    //Only images can be larger than the packet body length.
                    state = Server::SERVER_STATE::SENDING;//set sending state for as long as there are packets to transmit from the packet list.
                    log << "Server is now sending";
                    sprintf(messageBuff, "%d image packets are being sent", static_cast<int>(packetList->size()));
                    log << messageBuff;

                    while (i < packetList->size())
                    {
                        PacketDef* imagePacket = packetList->at(i);
                        bytesRead = 0;

                        int bytesToSend = imagePacket->Serialize(sendBuffer);  // Send image packet after packet, and expect ACK responses for each
                        int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
                        log.writeToFileLog(sendingMsg, strlen(receivingMsg));
                        log.writeToFileLog(sendBuffer, bytesToSend);

                        if (sendResult > 0)
                        {
                            bytesRead = recvfrom(connectionDetails.socket, recvBuffer, PacketData::Constants::MAX_PACKET_LENGTH, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), &addrLength);
                            log.writeToFileLog(receivingMsg, strlen(receivingMsg));
                            log.writeToFileLog(recvBuffer, bytesRead);

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
                            log << "Failed to send image: 3 missed ACKS";

                            shutdown = true;
                            break;
                        }
                    }

                    break;
                }
            }
        }
        catch (...)
        {
            //error, tell the client to shutdown
            std::cout << "Fatal error. Shutting down" << std::endl;
            break;
        }
        
        //lastly, after processing all requests and sending all image packets, return to the idle state
        state = Server::SERVER_STATE::IDLE;
        Sleep(250);


    }

    //send command to shutdown

    int bytesToSend = shutdownResponse.Serialize(sendBuffer);  // SEND shutdown
    int sendResult = sendto(connectionDetails.socket, sendBuffer, bytesToSend, NULL, reinterpret_cast<struct sockaddr*>(&rxSender), addrLength);
    log << "Shutting down";


    shutdown = true;
    timerThread.join();

    return 1;
}

int Server::menuThread(Menu& menu, bool& shutdown)
{
    while (!shutdown)
    {
        menu.printMenu();
        Sleep(500);
    }
    return 0;
}
