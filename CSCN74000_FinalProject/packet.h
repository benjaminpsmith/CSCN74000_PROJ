#ifndef HPP_PACKET
#define HPP_PACKET

#include <cstdint>
#include <cstring>
#include <iostream>
#include "position.h"

namespace PacketData {

    class Constants {
    public:
        static constexpr unsigned int MAX_HEADER_LENGTH = 21;  // This may not be accurate due to byte padding and alignment
        static constexpr unsigned int MAX_BODY_LENGTH = 255;
        static constexpr unsigned int MAX_TAIL_LENGTH = 4;
        static constexpr unsigned int MIN_PACKET_LENGTH = MAX_HEADER_LENGTH + MAX_TAIL_LENGTH;
        static constexpr unsigned int MAX_PACKET_LENGTH = MAX_HEADER_LENGTH + MAX_BODY_LENGTH + MAX_TAIL_LENGTH;
    };

    class PacketDef
    {
    public:
        enum Flag : uint8_t {
            EMPTY = 0,
            BB = 1,//000001
            IMG = 2,//000010
            AUTH = 4,//000100
            ACK = 8,//001000
            AUTH_ACK = 12,//
            AUTH_LOST = 128,
            SHUTDOWN = 255//11111111
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
        const std::string error_msg = "An error has occured.";

    public:
        // Constructor and Destructor
        PacketDef() : PACKET{ 0, 0, Flag::EMPTY, 0, 0, 0, nullptr, 0 } {}
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
        PacketDef(const char* rawData, int length)
            : PACKET{ 0, 0, Flag::EMPTY, 0, 0, 0, nullptr, 0 } {

            int status = SUCC;

            if (rawData == nullptr || length < Constants::MIN_PACKET_LENGTH) {
                throw std::invalid_argument(error_msg);
            }

            size_t offset = 0;

            // Deserialize header fields
            if (memcpy(&PACKET.HEADER.src, &rawData[offset], sizeof(PACKET.HEADER.src)) != &PACKET.HEADER.src) { throw std::runtime_error(error_msg); }
            offset += sizeof(PACKET.HEADER.src);

            if (memcpy(&PACKET.HEADER.dest, &rawData[offset], sizeof(PACKET.HEADER.dest)) != &PACKET.HEADER.dest) { throw std::runtime_error(error_msg); }
            offset += sizeof(PACKET.HEADER.dest);

            if (memcpy(&PACKET.HEADER.flag, &rawData[offset], sizeof(PACKET.HEADER.flag)) != &PACKET.HEADER.flag) { throw std::runtime_error(error_msg); }
            offset += sizeof(PACKET.HEADER.flag);

            if (memcpy(&PACKET.HEADER.seqNum, &rawData[offset], sizeof(PACKET.HEADER.seqNum)) != &PACKET.HEADER.seqNum) { throw std::runtime_error(error_msg); }
            offset += sizeof(PACKET.HEADER.seqNum);

            if (memcpy(&PACKET.HEADER.totalCount, &rawData[offset], sizeof(PACKET.HEADER.totalCount)) != &PACKET.HEADER.totalCount) { throw std::runtime_error(error_msg); }
            offset += sizeof(PACKET.HEADER.totalCount);

            if (memcpy(&PACKET.HEADER.bodyLen, &rawData[offset], sizeof(PACKET.HEADER.bodyLen)) != &PACKET.HEADER.bodyLen) { throw std::runtime_error(error_msg); }
            offset += sizeof(PACKET.HEADER.bodyLen);

            // Deserialize body
            if (PACKET.HEADER.bodyLen > 0) {
                PACKET.BODY.data = new char[PACKET.HEADER.bodyLen];
                //if (PACKET.BODY.data == nullptr) {
                //    return; // Memory allocation failure
                //}

                if (memcpy(PACKET.BODY.data, &rawData[offset], PACKET.HEADER.bodyLen) != PACKET.BODY.data) {
                    delete[] PACKET.BODY.data;
                    PACKET.BODY.data = nullptr;
                    throw std::runtime_error(error_msg);
                }
                offset += PACKET.HEADER.bodyLen;
            }

            // Deserialize tail
            if (memcpy(&PACKET.TAIL.crc, &rawData[offset], sizeof(PACKET.TAIL.crc)) != &PACKET.TAIL.crc) {
                if (PACKET.BODY.data != nullptr) {
                    delete[] PACKET.BODY.data;
                    PACKET.BODY.data = nullptr;
                }
                throw std::runtime_error(error_msg);
            }

        }
        ~PacketDef() {
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

            int status = SUCC;

            // Delete old data if it exists
            if (PACKET.BODY.data != nullptr) {
                delete[] PACKET.BODY.data;
                PACKET.BODY.data = nullptr;
            }

            if (bytes > Constants::MAX_BODY_LENGTH) {
                status = ERR; // Data too large
            }

            // Allocate memory for new data
            PACKET.BODY.data = new char[bytes];
            if (PACKET.BODY.data == nullptr) {
                status = ERR; // Memory allocation failure
            }

            // Initialize memory to 0
            if (memset(PACKET.BODY.data, 0, bytes) != PACKET.BODY.data) {
                delete[] PACKET.BODY.data;
                PACKET.BODY.data = nullptr;
                status = ERR; // memset failure
            }

            // Copy data
            if (memcpy(PACKET.BODY.data, data, bytes) != PACKET.BODY.data) {
                delete[] PACKET.BODY.data;
                PACKET.BODY.data = nullptr;
                status = ERR; // memcpy failure
            }

            PACKET.HEADER.bodyLen = static_cast<unsigned int>(bytes);
            return status; // Success
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

            int packetSize = SUCC;

            if (outBuffer == nullptr) {
                packetSize = ERR; // Invalid output buffer
            }

            size_t offset = 0;

            // Calculate the size of the entire packet
            packetSize = sizeof(PACKET.HEADER.src) + sizeof(PACKET.HEADER.dest) + sizeof(PACKET.HEADER.flag) +
                sizeof(PACKET.HEADER.seqNum) + sizeof(PACKET.HEADER.totalCount) + sizeof(PACKET.HEADER.bodyLen) +
                PACKET.HEADER.bodyLen + sizeof(PACKET.TAIL.crc);

            if (packetSize > Constants::MAX_PACKET_LENGTH) {
                packetSize = ERR; // Packet too large
            }

            // Initialize the buffer to 0
            if (memset(outBuffer, 0, Constants::MAX_PACKET_LENGTH) != outBuffer) {
                packetSize = ERR; // memset failure
            }

            // Serialize header fields
            if (memcpy(&outBuffer[offset], &PACKET.HEADER.src, sizeof(PACKET.HEADER.src)) != &outBuffer[offset]) { packetSize = ERR; }
            offset += sizeof(PACKET.HEADER.src);

            if (memcpy(&outBuffer[offset], &PACKET.HEADER.dest, sizeof(PACKET.HEADER.dest)) != &outBuffer[offset]) { packetSize = ERR; }
            offset += sizeof(PACKET.HEADER.dest);

            if (memcpy(&outBuffer[offset], &PACKET.HEADER.flag, sizeof(PACKET.HEADER.flag)) != &outBuffer[offset]) { packetSize = ERR; }
            offset += sizeof(PACKET.HEADER.flag);

            if (memcpy(&outBuffer[offset], &PACKET.HEADER.seqNum, sizeof(PACKET.HEADER.seqNum)) != &outBuffer[offset]) { packetSize = ERR; }
            offset += sizeof(PACKET.HEADER.seqNum);

            if (memcpy(&outBuffer[offset], &PACKET.HEADER.totalCount, sizeof(PACKET.HEADER.totalCount)) != &outBuffer[offset]) { packetSize = ERR; }
            offset += sizeof(PACKET.HEADER.totalCount);

            if (memcpy(&outBuffer[offset], &PACKET.HEADER.bodyLen, sizeof(PACKET.HEADER.bodyLen)) != &outBuffer[offset]) { packetSize = ERR; }
            offset += sizeof(PACKET.HEADER.bodyLen);

            // Serialize body
            if (memcpy(&outBuffer[offset], PACKET.BODY.data, PACKET.HEADER.bodyLen) != &outBuffer[offset]) { packetSize = ERR; }
            offset += PACKET.HEADER.bodyLen;

            // Serialize tail
            if (memcpy(&outBuffer[offset], &PACKET.TAIL.crc, sizeof(PACKET.TAIL.crc)) != &outBuffer[offset]) { packetSize = ERR; }

            return packetSize; // Success
        }

        // Extra Functions
        int operator=(const PacketDef& other) {

            int status = SUCC;

            if (this != &other) {

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
                        status = ERR; // Memory allocation failure
                    }

                    // Initialize memory to 0
                    if (memset(this->PACKET.BODY.data, 0, bodyLen) != this->PACKET.BODY.data) {
                        delete[] this->PACKET.BODY.data;
                        this->PACKET.BODY.data = nullptr;
                        status = ERR; // memset failure
                    }

                    // Copy body data
                    if (memcpy(this->PACKET.BODY.data, other.PACKET.BODY.data, bodyLen) != this->PACKET.BODY.data) {
                        delete[] this->PACKET.BODY.data;
                        this->PACKET.BODY.data = nullptr;
                        status = ERR; // memcpy failure
                    }
                }
                else {
                    this->PACKET.BODY.data = nullptr;
                }

                // Copy tail fields
                this->PACKET.TAIL.crc = other.PACKET.TAIL.crc;
            }
            return status; // Success
        }
    };
}

#endif // !HPP_PACKET