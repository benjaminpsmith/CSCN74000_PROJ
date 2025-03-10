#include <cstdint>
using namespace std;

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
            uint32_t src;   
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
};