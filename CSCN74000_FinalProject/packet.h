#ifndef HPP_PACKET
#define HPP_PACKET

#include <cstdint>
#include <cstring>
#include <iostream>
#include "position.h"
using namespace std;

const unsigned int MAX_HEADER_LENGTH = 21;  // This may not be accurate due to byte padding and alignment
const unsigned int MAX_BODY_LENGTH = 256;
const unsigned int MAX_TAIL_LENGTH = 4;
const unsigned int MIN_PACKET_LENGTH = MAX_HEADER_LENGTH + MAX_TAIL_LENGTH;
const unsigned int MAX_PACKET_LENGTH = MAX_HEADER_LENGTH + MAX_BODY_LENGTH + MAX_TAIL_LENGTH;

class PacketDef
{
public:
    enum Flag : uint8_t {
        EMPTY = 0,
        BB = 1,//000001
        IMG = 2,//000010
        AUTH = 4,//000100
        ACK = 8,//001000
        AUTH_ACK = 12,//001100
    };

private:

    struct packet {

        struct header {
            // Uses ICAO Aircraft Address for IDs as the client (plane) (24-bit hexadecimal) e.g. A0B1C2 - Ignore last byte (8 bits)
            uint32_t src;
            uint32_t dest;
            Flag flag;
            unsigned int seqNum;
            unsigned int totalCount;
            unsigned int bodyLen;
        }HEADER;

        struct body {
            char* data;
        }BODY;

        struct tail {
            unsigned int crc;
        }TAIL;
    }PACKET;

    const int ERR = -1;
    const int SUCC = 1;

public:
    // Constructor and Destructor
    PacketDef() {

        PACKET.HEADER.src = '0';
        PACKET.HEADER.dest = '0';
        PACKET.HEADER.flag = Flag::EMPTY;
        PACKET.HEADER.seqNum = 0;
        PACKET.HEADER.totalCount = 0;
        PACKET.HEADER.bodyLen = 0;

        PACKET.BODY.data = nullptr;

        PACKET.TAIL.crc = 0;
    }
    PacketDef(uint32_t src, uint32_t dest, Flag flag, unsigned int seqNum, unsigned int totalCount) {

        PACKET.HEADER.src = src;
        PACKET.HEADER.dest = dest;
        PACKET.HEADER.flag = flag;
        PACKET.HEADER.seqNum = seqNum;
        PACKET.HEADER.totalCount = totalCount;
        PACKET.HEADER.bodyLen = 0;

        PACKET.BODY.data = nullptr;

        PACKET.TAIL.crc = 0;
    }
    PacketDef(const char* rawData, int length) { // This is the "deserialize" function
        size_t offset = 0;

        // Initialize default values
        PACKET.HEADER.src = '0';    // default src/dest
        PACKET.HEADER.dest = '0';   // default src/dest
        PACKET.HEADER.flag = Flag::EMPTY;
        PACKET.HEADER.seqNum = 0;
        PACKET.HEADER.totalCount = 0;
        PACKET.HEADER.bodyLen = 0;

        PACKET.BODY.data = nullptr;
        PACKET.TAIL.crc = 0;

        // Validate input length
        if (length < MIN_PACKET_LENGTH || length <= 0) {
            if (PACKET.BODY.data != nullptr) {
                delete[] PACKET.BODY.data;
                PACKET.BODY.data = nullptr;
            }
            return;
        }

        // Header
        // Src
        if (memcpy(&PACKET.HEADER.src, rawData + offset, sizeof(PACKET.HEADER.src)) != &PACKET.HEADER.src) {
            return; // Handle error
        }
        offset += sizeof(PACKET.HEADER.src);

        // Dest
        if (memcpy(&PACKET.HEADER.dest, rawData + offset, sizeof(PACKET.HEADER.dest)) != &PACKET.HEADER.dest) {
            return; // Handle error
        }
        offset += sizeof(PACKET.HEADER.dest);

        // Flag
        if (memcpy(&PACKET.HEADER.flag, rawData + offset, sizeof(PACKET.HEADER.flag)) != &PACKET.HEADER.flag) {
            return; // Handle error
        }
        offset += sizeof(PACKET.HEADER.flag);

        // Seq. Number
        if (memcpy(&PACKET.HEADER.seqNum, rawData + offset, sizeof(PACKET.HEADER.seqNum)) != &PACKET.HEADER.seqNum) {
            return; // Handle error
        }
        offset += sizeof(PACKET.HEADER.seqNum);

        // Total Count
        if (memcpy(&PACKET.HEADER.totalCount, rawData + offset, sizeof(PACKET.HEADER.totalCount)) != &PACKET.HEADER.totalCount) {
            return; // Handle error
        }
        offset += sizeof(PACKET.HEADER.totalCount);

        // Body Length
        if (memcpy(&PACKET.HEADER.bodyLen, rawData + offset, sizeof(PACKET.HEADER.bodyLen)) != &PACKET.HEADER.bodyLen) {
            return; // Handle error
        }
        offset += sizeof(PACKET.HEADER.bodyLen);

        // Body
        // Data
        if (PACKET.HEADER.bodyLen > 0) {
            PACKET.BODY.data = new char[PACKET.HEADER.bodyLen]; // Allocate memory for body data
            if (PACKET.BODY.data == nullptr) {
                return; // Handle memory allocation failure
            }

            // Initialize memory to 0
            if (memset(PACKET.BODY.data, 0, PACKET.HEADER.bodyLen) != PACKET.BODY.data) {
                delete[] PACKET.BODY.data; // Clean up on failure
                PACKET.BODY.data = nullptr;
                return; // Handle error
            }

            // Copy body data
            if (memcpy(PACKET.BODY.data, rawData + offset, PACKET.HEADER.bodyLen) != PACKET.BODY.data) {
                delete[] PACKET.BODY.data; // Clean up on failure
                PACKET.BODY.data = nullptr;
                return; // Handle error
            }
            offset += PACKET.HEADER.bodyLen;
        }

        // Tail
        // CRC
        if (memcpy(&PACKET.TAIL.crc, rawData + offset, sizeof(PACKET.TAIL.crc)) != &PACKET.TAIL.crc) {
            if (PACKET.BODY.data != nullptr) {
                delete[] PACKET.BODY.data; // Clean up on failure
                PACKET.BODY.data = nullptr;
            }
            return; // Handle error
        }

        // If a failure, set everything to the defaults and return
    }
    ~PacketDef() {
        if (PACKET.BODY.data != nullptr)
            delete[]PACKET.BODY.data;
    }

    // Setters and Getters
    void setSrc(uint32_t src) { PACKET.HEADER.src = src; }
    uint32_t getSrc() const { return PACKET.HEADER.src; }

    void setDest(uint32_t dest) { PACKET.HEADER.dest = dest; }
    uint32_t getDest() const { return PACKET.HEADER.dest; }

    void setFlag(Flag flag) { PACKET.HEADER.flag = flag; }
    Flag getFlag() const { return PACKET.HEADER.flag; }

    void setSeqNum(unsigned int seqNum) { PACKET.HEADER.seqNum = seqNum; }
    unsigned int getSeqNum() const { return PACKET.HEADER.seqNum; }

    void setTotalCount(unsigned int totalCount) { PACKET.HEADER.totalCount = totalCount; }
    unsigned int getTotalCount() const { return PACKET.HEADER.totalCount; }

    void setBodyLen(unsigned int bodyLen) { PACKET.HEADER.bodyLen = bodyLen; }
    unsigned int getBodyLen() const { return PACKET.HEADER.bodyLen; }

    // Special getter and setter due to allocated memory
    int setData(const char* data, size_t bytes) {

        // Delete old data if it is not already nullptr
        if (PACKET.BODY.data) {
            delete[]PACKET.BODY.data;
            PACKET.BODY.data = nullptr;
        }

        if (bytes > MAX_BODY_LENGTH) {
            return ERR;
        }

        // If no data, set body length to 0
        if (bytes <= 0) {
            PACKET.HEADER.bodyLen = 0;
            return ERR;
        }

        // Allocate memory for new data and check for failures. If failure, clean up.
        PACKET.BODY.data = new char[bytes];
        if (memset(PACKET.BODY.data, 0, bytes) != PACKET.BODY.data) {
            delete[] PACKET.BODY.data;
            PACKET.BODY.data = nullptr;
            return ERR;
        }
        if (memcpy(PACKET.BODY.data, data, bytes) != PACKET.BODY.data) {
            delete[] PACKET.BODY.data;
            PACKET.BODY.data = nullptr;
            return ERR;
        }

        PACKET.HEADER.bodyLen = (unsigned int)bytes;
    }
    char* getData() const { return PACKET.BODY.data; }

    // Setters and Getters cont.
    void setCrc(unsigned int crc) { PACKET.TAIL.crc = crc; }
    unsigned int getCrc() const { return PACKET.TAIL.crc; }

    /// <summary>
    /// Serializes the packet into a buffer that was given as an argument.
    /// </summary>
    /// <param name="outBuffer"></param>
    /// <returns>The size of the packet, or -1 if an error occured.</returns>
    int Serialize(char* outBuffer) {

        int bytesSerialized = 0;
        size_t offset = 0; // How far "over" we need to offset our memcpy by when serializing

        // Calculate the size of the entire packet
        int packetSize = sizeof(PACKET.HEADER.src) + sizeof(PACKET.HEADER.dest) + sizeof(PACKET.HEADER.flag) + sizeof(PACKET.HEADER.seqNum) + sizeof(PACKET.HEADER.totalCount) + sizeof(PACKET.HEADER.bodyLen) +
            PACKET.HEADER.bodyLen +
            sizeof(PACKET.TAIL.crc);

        if (packetSize > MAX_PACKET_LENGTH)
            return ERR;

        if (memset(outBuffer, 0, MAX_PACKET_LENGTH) != outBuffer) { return ERR; }

        // Src
        if (memcpy(outBuffer + offset, &PACKET.HEADER.src, sizeof(PACKET.HEADER.src)) != outBuffer + offset) { return ERR; }
        offset += sizeof(PACKET.HEADER.src);
        // Dest
        if (memcpy(outBuffer + offset, &PACKET.HEADER.dest, sizeof(PACKET.HEADER.dest)) != outBuffer + offset) { return ERR; }
        offset += sizeof(PACKET.HEADER.dest);
        // Flag
        if (memcpy(outBuffer + offset, &PACKET.HEADER.flag, sizeof(PACKET.HEADER.flag)) != outBuffer + offset) { return ERR; }
        offset += sizeof(PACKET.HEADER.flag);
        // Seq. Num
        if (memcpy(outBuffer + offset, &PACKET.HEADER.seqNum, sizeof(PACKET.HEADER.seqNum)) != outBuffer + offset) { return ERR; }
        offset += sizeof(PACKET.HEADER.seqNum);
        // Total Count
        if (memcpy(outBuffer + offset, &PACKET.HEADER.totalCount, sizeof(PACKET.HEADER.totalCount)) != outBuffer + offset) { return ERR; }
        offset += sizeof(PACKET.HEADER.totalCount);
        // Body Length
        if (memcpy(outBuffer + offset, &PACKET.HEADER.bodyLen, sizeof(PACKET.HEADER.bodyLen)) != outBuffer + offset) { return ERR; }
        offset += sizeof(PACKET.HEADER.bodyLen);

        // Body
        if (memcpy(outBuffer + offset, PACKET.BODY.data, PACKET.HEADER.bodyLen) != outBuffer + offset) { return ERR; }
        offset += PACKET.HEADER.bodyLen;

        // CRC
        if (memcpy(outBuffer + offset, &PACKET.TAIL.crc, sizeof(PACKET.TAIL.crc)) != outBuffer + offset) { return ERR; }

        return packetSize;
    }

    // Extra Functions
    void display() const {
        cout << "Packet Information:" << endl;
        cout << "-------------------" << endl;
        cout << "Source: " << hex << PACKET.HEADER.src << dec << endl;
        cout << "Destination: " << hex << PACKET.HEADER.dest << dec << endl;
        cout << "Flag: " << static_cast<int>(PACKET.HEADER.flag) << endl;
        cout << "Sequence Number: " << PACKET.HEADER.seqNum << endl;
        cout << "Total Count: " << PACKET.HEADER.totalCount << endl;
        cout << "Body Length: " << PACKET.HEADER.bodyLen << endl;

        cout << "Data: ";
        if (PACKET.BODY.data && PACKET.HEADER.bodyLen > 0) {
            for (unsigned int i = 0; i < PACKET.HEADER.bodyLen; ++i) {
                cout << PACKET.BODY.data[i];
            }
        }
        else {
            cout << "(empty)";
        }
        cout << endl;

        cout << "CRC: " << PACKET.TAIL.crc << endl;
    }

    int operator=(const PacketDef& other) {
        if (this == &other) {
            return SUCC;
        }

        // Copy header fields
        this->PACKET.HEADER.bodyLen = other.PACKET.HEADER.bodyLen;
        this->PACKET.HEADER.dest = other.PACKET.HEADER.dest;
        this->PACKET.HEADER.flag = other.PACKET.HEADER.flag;
        this->PACKET.HEADER.seqNum = other.PACKET.HEADER.seqNum;
        this->PACKET.HEADER.src = other.PACKET.HEADER.src;
        this->PACKET.HEADER.totalCount = other.PACKET.HEADER.totalCount;

        unsigned int bodyLen = this->PACKET.HEADER.bodyLen;

        // Delete existing data if it exists
        if (this->PACKET.BODY.data != nullptr) {
            delete[] this->PACKET.BODY.data;
            this->PACKET.BODY.data = nullptr;
        }

        // Allocate new memory for body data
        if (bodyLen > 0) {
            this->PACKET.BODY.data = new char[bodyLen];
            if (this->PACKET.BODY.data == nullptr) {
                return ERR; // Memory allocation failure
            }

            // Initialize memory to 0
            if (memset(this->PACKET.BODY.data, 0, bodyLen) != this->PACKET.BODY.data) {
                delete[] this->PACKET.BODY.data;
                this->PACKET.BODY.data = nullptr;
                return ERR; // memset failure
            }

            // Copy body data
            if (memcpy(this->PACKET.BODY.data, other.PACKET.BODY.data, bodyLen) != this->PACKET.BODY.data) {
                delete[] this->PACKET.BODY.data;
                this->PACKET.BODY.data = nullptr;
                return ERR; // memcpy failure
            }
        }
        else {
            this->PACKET.BODY.data = nullptr;
        }

        // Copy tail fields
        this->PACKET.TAIL.crc = other.PACKET.TAIL.crc;

        return SUCC; // Success
    }
};

#endif 