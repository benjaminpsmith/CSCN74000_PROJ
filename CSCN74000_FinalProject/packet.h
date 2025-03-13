#ifndef HPP_PACKET
#define HPP_PACKET

#include <cstdint>
#include <cstring>
#include <iostream>
using namespace std;

const unsigned int MAX_HEADER_LENGTH = 21;  // This may not be accurate due to byte padding and alignment
const unsigned int MAX_BODY_LENGTH = 256;
const unsigned int MAX_TAIL_LENGTH = 4;
const unsigned int MIN_PACKET_LENGTH = MAX_HEADER_LENGTH + MAX_TAIL_LENGTH;
const unsigned int MAX_PACKET_LENGTH = MAX_HEADER_LENGTH + MAX_BODY_LENGTH + MAX_TAIL_LENGTH;

class PacketDef
{
public:
    enum Flag : uint8_t{
        EMPTY = 0,
        BB = 1,
        IMG = 2,
        AUTH = 3,
        ACK = 4,
        AUTH_ACK = 12,
    };

private:

    struct packet{

        struct header{
            // Uses ICAO Aircraft Address for IDs as the client (plane) (24-bit hexadecimal) e.g. A0B1C2 - Ignore last byte (8 bits)
            // Use random 3 bytes for server address
            uint32_t src;   // liable to change
            uint32_t dest;  
            Flag flag;
            unsigned int seqNum;
            unsigned int totalCount;
            unsigned int bodyLen;
        }HEADER;

        struct body{
            char* data;
        }BODY;

        struct tail{
            unsigned int crc;
        }TAIL;
    }PACKET;

    char* outBuffer; // What data will be stored in after being serialized

public:
    // Constructor and Destructor
    PacketDef(){

        PACKET.HEADER.src = '0';
		PACKET.HEADER.dest = '0';
		PACKET.HEADER.flag = Flag::EMPTY;
        PACKET.HEADER.seqNum = 0;
        PACKET.HEADER.totalCount = 0;
		PACKET.HEADER.bodyLen = 0;

		PACKET.BODY.data = nullptr;

		PACKET.TAIL.crc = 0;

        this->outBuffer = nullptr;

    }
    PacketDef(const char* rawData, int length){ // This is the "deserialize" function

        size_t offset = 0;

        PACKET.HEADER.src = '0';    // default src/dest?
        PACKET.HEADER.dest = '0';   // default src/dest?
        PACKET.HEADER.flag = Flag::EMPTY;
        PACKET.HEADER.seqNum = 0;
        PACKET.HEADER.totalCount = 0;
        PACKET.HEADER.bodyLen = 0;

        PACKET.BODY.data = nullptr;
        this->outBuffer = nullptr;

        PACKET.TAIL.crc = 0;

        if (length < MIN_PACKET_LENGTH || length <= 0)
        {
            if (PACKET.BODY.data != nullptr)
                delete[] PACKET.BODY.data;

            return;
        }


        // Header
        // Src
        memcpy(&PACKET.HEADER.src, rawData + offset, sizeof(PACKET.HEADER.src));
        offset += sizeof(PACKET.HEADER.src);

        // Dest
        memcpy(&PACKET.HEADER.dest, rawData + offset, sizeof(PACKET.HEADER.dest));
        offset += sizeof(PACKET.HEADER.dest);

        // Flag
        memcpy(&PACKET.HEADER.flag, rawData + offset, sizeof(PACKET.HEADER.flag));
        offset += sizeof(PACKET.HEADER.flag);

        // Seq. Number
        memcpy(&PACKET.HEADER.seqNum, rawData + offset, sizeof(PACKET.HEADER.seqNum));
        offset += sizeof(PACKET.HEADER.seqNum);

        // Total Count
        memcpy(&PACKET.HEADER.totalCount, rawData + offset, sizeof(PACKET.HEADER.totalCount));
        offset += sizeof(PACKET.HEADER.totalCount);

        // Body Length
        memcpy(&PACKET.HEADER.bodyLen, rawData + offset, sizeof(PACKET.HEADER.bodyLen));
        offset += sizeof(PACKET.HEADER.bodyLen);

        // Body
        // Data
        if (this->PACKET.BODY.data == nullptr)
        {
            PACKET.BODY.data = new char[PACKET.HEADER.bodyLen]; // Could check for bad allocation
        }
        else
        {
            delete[] PACKET.BODY.data;
            PACKET.BODY.data = new char[PACKET.HEADER.bodyLen]; // Could check for bad allocation
        }

        memcpy(PACKET.BODY.data, rawData + offset, PACKET.HEADER.bodyLen);
        offset += PACKET.HEADER.bodyLen;

        // Tail
        // CRC
        memcpy(&PACKET.TAIL.crc, rawData + offset, sizeof(PACKET.TAIL.crc));
    }
    PacketDef(const PacketDef &srcPkt){ // Copy constructor
        // Header
        this->PACKET.HEADER.src = srcPkt.PACKET.HEADER.src;
		this->PACKET.HEADER.dest = srcPkt.PACKET.HEADER.dest;
		this->PACKET.HEADER.seqNum = srcPkt.PACKET.HEADER.seqNum;
		this->PACKET.HEADER.flag = srcPkt.PACKET.HEADER.flag;
		this->PACKET.HEADER.bodyLen = srcPkt.PACKET.HEADER.bodyLen;
		
		// Body
		if (srcPkt.PACKET.HEADER.bodyLen > 0) {
			this->PACKET.BODY.data = new char[srcPkt.PACKET.HEADER.bodyLen];
			memcpy(this->PACKET.BODY.data, srcPkt.PACKET.BODY.data, srcPkt.PACKET.HEADER.bodyLen);
		}
		else {
			this->PACKET.BODY.data = nullptr;
		}

		// Tail
		this->PACKET.TAIL.crc = srcPkt.PACKET.TAIL.crc;
		this->outBuffer = nullptr;

    }
    PacketDef(int preAllocationBytes)
    {
        this->PACKET.BODY.data = new char[preAllocationBytes];
    }
    ~PacketDef(){
        if (PACKET.BODY.data)
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
    void setData(const char* data, size_t bytes) {

        // Delete old data if it is not already nullptr
        if (PACKET.BODY.data) {
			delete[]PACKET.BODY.data;
			PACKET.BODY.data = nullptr;
		}

        if(bytes > MAX_BODY_LENGTH){
            return;
        }

        // If no data, set body length to 0
		if (bytes <= 0) {
			PACKET.HEADER.bodyLen = 0;
			return;
		}

        // Allocate memory for new data
		PACKET.BODY.data = new char[bytes];
		memcpy(PACKET.BODY.data, data, bytes);

		PACKET.HEADER.bodyLen = (unsigned int)bytes;
    }
    char* getData() const { return PACKET.BODY.data; }

    // Setters and Getters cont.
    void setCrc(unsigned int crc) { PACKET.TAIL.crc = crc; }
    unsigned int getCrc() const { return PACKET.TAIL.crc; }

    // Serialize
    int Serialize(char* outBuffer, int bufferSize){

        int bytesSerialized = 0;
        size_t offset = 0; // How far "over" we need to offset our memcpy by when serializing

        // Calculate the size of the entire packet
        int packetSize = sizeof(PACKET.HEADER.src) + sizeof(PACKET.HEADER.dest) + sizeof(PACKET.HEADER.flag) + sizeof(PACKET.HEADER.seqNum) + sizeof(PACKET.HEADER.totalCount) + sizeof(PACKET.HEADER.bodyLen) +
                    PACKET.HEADER.bodyLen + 
                    sizeof(PACKET.TAIL.crc);

        if (packetSize > bufferSize)
            return 0;

        memset(outBuffer, 0, bufferSize);

        // Packet Header
        // Src
        memcpy(outBuffer + offset, &PACKET.HEADER.src, sizeof(PACKET.HEADER.src));
        offset += sizeof(PACKET.HEADER.src);
        // Dest
        memcpy(outBuffer + offset, &PACKET.HEADER.dest, sizeof(PACKET.HEADER.dest));
        offset += sizeof(PACKET.HEADER.dest);
        // Flag
        memcpy(outBuffer + offset, &PACKET.HEADER.flag, sizeof(PACKET.HEADER.flag));
        offset += sizeof(PACKET.HEADER.flag);
        // Seq. Num
        memcpy(outBuffer + offset, &PACKET.HEADER.seqNum, sizeof(PACKET.HEADER.seqNum));
        offset += sizeof(PACKET.HEADER.seqNum);
        // Total Count
        memcpy(outBuffer + offset, &PACKET.HEADER.totalCount, sizeof(PACKET.HEADER.totalCount));
        offset += sizeof(PACKET.HEADER.totalCount);
        // Body Length
        memcpy(outBuffer + offset, &PACKET.HEADER.bodyLen, sizeof(PACKET.HEADER.bodyLen));
        offset += sizeof(PACKET.HEADER.bodyLen);
        // Packet Body
        // Body
        memcpy(outBuffer + offset, PACKET.BODY.data, PACKET.HEADER.bodyLen);
        offset += PACKET.HEADER.bodyLen;
        // Packet Tail
        // CRC
        memcpy(outBuffer + offset, &PACKET.TAIL.crc, sizeof(PACKET.TAIL.crc));
        offset += sizeof(PACKET.TAIL.crc);

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
        } else {
            cout << "(empty)";
        }
        cout << endl;
    
        cout << "CRC: " << PACKET.TAIL.crc << endl;
    }
};

#endif