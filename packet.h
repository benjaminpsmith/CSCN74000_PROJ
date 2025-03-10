#include <cstdint>
#include <iostream>
using namespace std;

const unsigned int MAX_BODY_LENGTH = 256;

class Packet
{
public:
    enum class Flag{
        BB = 1,
        IMG = 2,
        AUTH = 3,
        ACK = 4
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

public:
    // Constructor and Destructor
    Packet();
    Packet(char* rawData);
    Packet(const Packet &src);
    ~Packet();

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
    void setData(char* data, size_t bytes) {

        // Delete old data if it is not already nullptr
        if (PACKET.BODY.data) {
			delete[]PACKET.BODY.data;
			PACKET.BODY.data = nullptr;
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

    void setCrc(unsigned int crc) { PACKET.TAIL.crc = crc; }
    unsigned int getCrc() const { return PACKET.TAIL.crc; }

    // Serialize and Deserialize
};