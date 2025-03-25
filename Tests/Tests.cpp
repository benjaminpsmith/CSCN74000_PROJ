#include "pch.h"
#include "CppUnitTest.h"
#include <cstring>
#include <vector>
#include <filesystem>
#include "../CSCN74000_FinalProject/packet.h"


using namespace Microsoft::VisualStudio::CppUnitTestFramework;

#define ERROR_CODE -1
#define SUCCESS_CODE 1
#define POSITION_SIZE 40
#define POSITION_ATTR_COUNT 5

namespace Microsoft::VisualStudio::CppUnitTestFramework
{
    template<>
    std::wstring ToString<PacketData::PacketDef::Flag>(const PacketData::PacketDef::Flag& flag)
    {
        switch (flag)
        {
        case PacketData::PacketDef::Flag::EMPTY: return L"EMPTY";
        case PacketData::PacketDef::Flag::BB: return L"BB";
        case PacketData::PacketDef::Flag::IMG: return L"IMG";
        case PacketData::PacketDef::Flag::AUTH: return L"AUTH";
        case PacketData::PacketDef::Flag::ACK: return L"ACK";
        case PacketData::PacketDef::Flag::AUTH_ACK: return L"AUTH_ACK";
        default: return L"UNKNOWN_FLAG";
        }
    }
}

namespace PacketDataTests
{
    TEST_CLASS(PacketDefTests)
    {
    public:
        TEST_METHOD(TestDefaultConstructor)
        {
            PacketData::PacketDef packet;

            // Verify default values
            Assert::AreEqual(0u, packet.getSrc(), L"Default source should be 0");
            Assert::AreEqual(0u, packet.getDest(), L"Default destination should be 0");
            Assert::AreEqual(PacketData::PacketDef::Flag::EMPTY, packet.getFlag(),
                L"Default flag should be EMPTY");
            Assert::AreEqual(0u, packet.getSeqNum(), L"Default sequence number should be 0");
            Assert::AreEqual(0u, packet.getTotalCount(), L"Default total count should be 0");
            Assert::AreEqual(0u, packet.getBodyLen(), L"Default body length should be 0");
            Assert::IsNull(packet.getData(), L"Default data should be null");
            Assert::AreEqual(0u, packet.getCrc(), L"Default CRC should be 0");
        }
        TEST_METHOD(TestParameterizedConstructor)
        {
            uint32_t src = 0xA0B1C2;
            uint32_t dest = 0xD3E4F5;
            PacketData::PacketDef::Flag flag = PacketData::PacketDef::Flag::BB;
            unsigned int seqNum = 1;
            unsigned int totalCount = 10;

            PacketData::PacketDef packet(src, dest, flag, seqNum, totalCount);

            Assert::AreEqual(src, packet.getSrc(), L"Source not set correctly");
            Assert::AreEqual(dest, packet.getDest(), L"Destination not set correctly");
            Assert::AreEqual(flag, packet.getFlag(), L"Flag not set correctly");
            Assert::AreEqual(seqNum, packet.getSeqNum(), L"Sequence number not set correctly");
            Assert::AreEqual(totalCount, packet.getTotalCount(), L"Total count not set correctly");
        }
        TEST_METHOD(TestRawDataConstructor)
        {
            // Prepare test packet data
            uint32_t src = 0xA0B1C2;
            uint32_t dest = 0xD3E4F5;
            PacketData::PacketDef::Flag flag = PacketData::PacketDef::Flag::BB;
            unsigned int seqNum = 1;
            unsigned int totalCount = 10;
            unsigned int bodyLen = 20;
            unsigned int crc = 0x12345678;

            // Create buffer to simulate raw packet data
            char buffer[PacketData::Constants::MAX_PACKET_LENGTH];
            size_t offset = 0;

            // Serialize test data into buffer
            std::memcpy(&buffer[offset], &src, sizeof(src));
            offset += sizeof(src);

            std::memcpy(&buffer[offset], &dest, sizeof(dest));
            offset += sizeof(dest);

            std::memcpy(&buffer[offset], &flag, sizeof(flag));
            offset += sizeof(flag);

            std::memcpy(&buffer[offset], &seqNum, sizeof(seqNum));
            offset += sizeof(seqNum);

            std::memcpy(&buffer[offset], &totalCount, sizeof(totalCount));
            offset += sizeof(totalCount);

            std::memcpy(&buffer[offset], &bodyLen, sizeof(bodyLen));
            offset += sizeof(bodyLen);

            // Add body data
            char bodyData[20];
            std::memset(bodyData, 0xAA, bodyLen);
            std::memcpy(&buffer[offset], bodyData, bodyLen);
            offset += bodyLen;

            std::memcpy(&buffer[offset], &crc, sizeof(crc));

            // Create packet from raw data
            PacketData::PacketDef packet(buffer, PacketData::Constants::MAX_PACKET_LENGTH);

            // Verify deserialized data
            Assert::AreEqual(src, packet.getSrc(), L"Source not deserialized correctly");
            Assert::AreEqual(dest, packet.getDest(), L"Destination not deserialized correctly");
            Assert::AreEqual(flag, packet.getFlag(), L"Flag not deserialized correctly");
            Assert::AreEqual(seqNum, packet.getSeqNum(), L"Sequence number not deserialized correctly");
            Assert::AreEqual(totalCount, packet.getTotalCount(), L"Total count not deserialized correctly");
            Assert::AreEqual(bodyLen, packet.getBodyLen(), L"Body length not deserialized correctly");
            Assert::AreEqual(crc, packet.getCrc(), L"CRC not deserialized correctly");
        }
        TEST_METHOD(TestRawDataConstructor_InvalidArguments)
        {
            // Test null rawData
            auto func1 = [] {
                PacketData::PacketDef packet(nullptr, 50);
                };
            Assert::ExpectException<std::invalid_argument>(func1,
                L"Null data should throw invalid_argument");

            // Test length too small
            char buffer[PacketData::Constants::MIN_PACKET_LENGTH - 1];
            auto func2 = [&buffer] {
                PacketData::PacketDef packet(buffer, PacketData::Constants::MIN_PACKET_LENGTH - 1);
                };
            Assert::ExpectException<std::invalid_argument>(func2,
                L"Too small buffer should throw invalid_argument");
        }
        TEST_METHOD(TestSettersAndGetters)
        {
            PacketData::PacketDef packet;

            // Test each setter and corresponding getter
            packet.setSrc(0xA0B1C2);
            Assert::AreEqual(0xA0B1C2u, packet.getSrc(), L"Source not set correctly");

            packet.setDest(0xD3E4F5);
            Assert::AreEqual(0xD3E4F5u, packet.getDest(), L"Destination not set correctly");

            packet.setFlag(PacketData::PacketDef::Flag::IMG);
            Assert::AreEqual(PacketData::PacketDef::Flag::IMG, packet.getFlag(),
                L"Flag not set correctly");

            packet.setSeqNum(5);
            Assert::AreEqual(5u, packet.getSeqNum(), L"Sequence number not set correctly");

            packet.setTotalCount(20);
            Assert::AreEqual(20u, packet.getTotalCount(), L"Total count not set correctly");

            packet.setCrc(0x12345678);
            Assert::AreEqual(0x12345678u, packet.getCrc(), L"CRC not set correctly");
        }
        TEST_METHOD(TestSetData)
        {
            PacketData::PacketDef packet;

            // Create test data
            char testData[100];
            std::memset(testData, 0xAA, sizeof(testData));

            // Set data
            int result = packet.setData(testData, sizeof(testData));
            Assert::AreEqual(1, result, L"Data setting should succeed");

            // Verify data
            Assert::AreEqual(sizeof(testData), static_cast<size_t>(packet.getBodyLen()),
                L"Body length not set correctly");
            Assert::IsNotNull(packet.getData(), L"Data should not be null");
            Assert::IsTrue(std::memcmp(testData, packet.getData(), sizeof(testData)) == 0,
                L"Data not copied correctly");

            // Test setting oversized data
            char largeData[PacketData::Constants::MAX_BODY_LENGTH + 1];
            result = packet.setData(largeData, sizeof(largeData));
            Assert::AreEqual(ERROR_CODE, result, L"Oversized data should fail");
        }
        TEST_METHOD(TestSerialize)
        {
            // Create test packet
            PacketData::PacketDef packet(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            // Create test body data
            PositionData::Position pos(12.34, 56.78, 90.0, 450.0, 5000.0);
            char bodyData[POSITION_SIZE];
            pos.Serialize(bodyData);

            // Set body data and CRC
            packet.setData(bodyData, PositionData::Position::GetSerializedSize());
            packet.setCrc(0x12345678);

            // Serialize
            char buffer[PacketData::Constants::MAX_PACKET_LENGTH];
            int serializeResult = packet.Serialize(buffer);

            // Verify serialization
            Assert::IsTrue(serializeResult > 0, L"Serialization should succeed");
            Assert::IsTrue(serializeResult <= PacketData::Constants::MAX_PACKET_LENGTH,
                L"Serialized packet length should be within max length");

            // Deserialize and verify
            PacketData::PacketDef deserializedPacket(buffer, serializeResult);
            Assert::AreEqual(packet.getSrc(), deserializedPacket.getSrc(),
                L"Source not correctly serialized/deserialized");
            Assert::AreEqual(packet.getCrc(), deserializedPacket.getCrc(),
                L"CRC not correctly serialized/deserialized");
        }
        TEST_METHOD(TestSerialize_NullBuffer)
        {
            PacketData::PacketDef packet(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            // Attempt to serialize to null buffer
            int serializeResult = packet.Serialize(nullptr);

            // Verify error return
            Assert::AreEqual(-1, serializeResult,
                L"Serialization to null buffer should return error");
        }
        TEST_METHOD(TestAssignmentOperator)
        {
            // Create first packet
            PacketData::PacketDef packet1(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            // Create body data
            PositionData::Position pos(12.34, 56.78, 90.0, 450.0, 5000.0);
            char bodyData[POSITION_SIZE];
            pos.Serialize(bodyData);

            // Set body data and CRC
            packet1.setData(bodyData, PositionData::Position::GetSerializedSize());
            packet1.setCrc(0x12345678);

            // Create second packet and assign
            PacketData::PacketDef packet2;
            int assignResult = (packet2 = packet1);

            // Verify assignment
            Assert::AreEqual(SUCCESS_CODE, assignResult, L"Assignment should succeed");

            // Compare all fields
            Assert::AreEqual(packet1.getSrc(), packet2.getSrc(), L"Source not copied");
            Assert::AreEqual(packet1.getDest(), packet2.getDest(), L"Destination not copied");
            Assert::AreEqual(packet1.getFlag(), packet2.getFlag(), L"Flag not copied");
            Assert::AreEqual(packet1.getSeqNum(), packet2.getSeqNum(), L"Sequence number not copied");
            Assert::AreEqual(packet1.getTotalCount(), packet2.getTotalCount(), L"Total count not copied");
            Assert::AreEqual(packet1.getBodyLen(), packet2.getBodyLen(), L"Body length not copied");
            Assert::AreEqual(packet1.getCrc(), packet2.getCrc(), L"CRC not copied");

            // Compare body data
            Assert::IsTrue(std::memcmp(packet1.getData(), packet2.getData(),
                packet1.getBodyLen()) == 0, L"Body data not copied correctly");
        }
    };
}

namespace PositionDataTests
{
    TEST_CLASS(PositionTests)
    {
    public:
        TEST_METHOD(TestDefaultConstructor)
        {
            PositionData::Position pos;

            // Verify all values are initialized to 0.0
            Assert::AreEqual(0.0, pos.latitude, L"Latitude should be 0.0");
            Assert::AreEqual(0.0, pos.longitude, L"Longitude should be 0.0");
            Assert::AreEqual(0.0, pos.heading, L"Heading should be 0.0");
            Assert::AreEqual(0.0, pos.velocity, L"Velocity should be 0.0");
            Assert::AreEqual(0.0, pos.altitude, L"Altitude should be 0.0");
        }
        TEST_METHOD(TestParameterizedConstructor)
        {
            double lat = 40.7128;
            double lon = -74.0060;
            double hdg = 90.0;
            double vel = 500.0;
            double alt = 5000.0;

            PositionData::Position pos(lat, lon, hdg, vel, alt);

            Assert::AreEqual(lat, pos.latitude, L"Latitude not set correctly");
            Assert::AreEqual(lon, pos.longitude, L"Longitude not set correctly");
            Assert::AreEqual(hdg, pos.heading, L"Heading not set correctly");
            Assert::AreEqual(vel, pos.velocity, L"Velocity not set correctly");
            Assert::AreEqual(alt, pos.altitude, L"Altitude not set correctly");
        }
        TEST_METHOD(TestRawDataConstructor)
        {
            // Prepare test data
            double testData[POSITION_ATTR_COUNT] = { 40.7128, -74.0060, 90.0, 500.0, 5000.0 };

            // Convert to char array to mimic raw data
            char rawData[POSITION_SIZE];
            std::memcpy(rawData, testData, sizeof(rawData));

            // Create position from raw data
            PositionData::Position pos(rawData);

            // Verify data is correctly deserialized
            Assert::AreEqual(testData[0], pos.latitude, L"Latitude not deserialized correctly");
            Assert::AreEqual(testData[1], pos.longitude, L"Longitude not deserialized correctly");
            Assert::AreEqual(testData[2], pos.heading, L"Heading not deserialized correctly");
            Assert::AreEqual(testData[3], pos.velocity, L"Velocity not deserialized correctly");
            Assert::AreEqual(testData[4], pos.altitude, L"Altitude not deserialized correctly");
        }
        TEST_METHOD(TestRawDataConstructor_NullPointer)
        {
            // Verify that passing nullptr throws an invalid_argument exception
            auto testFunc = [] {
                PositionData::Position pos(nullptr);
                };

            Assert::ExpectException<std::invalid_argument>(testFunc, L"Null pointer should throw invalid_argument");
        }
        TEST_METHOD(TestWriteToFile)
        {
            // Prepare test position
            PositionData::Position pos(40.7128, -74.0060, 90.0, 500.0, 5000.0);

            // Unique test file path
            std::string testFilePath = "test_position_output.txt";

            // Remove file if it exists
            if (std::filesystem::exists(testFilePath)) {
                std::filesystem::remove(testFilePath);
            }

            // Write to file
            bool writeResult = pos.writeToFile(testFilePath);
            Assert::IsTrue(writeResult, L"File write should succeed");

            // Verify file exists and is not empty
            Assert::IsTrue(std::filesystem::exists(testFilePath), L"File should be created");

            // Optional: Read file contents to verify
            std::ifstream file(testFilePath);
            std::string line;
            std::getline(file, line);
            file.close();

            // Clean up
            std::filesystem::remove(testFilePath);

            // Basic check on written data
            Assert::IsFalse(line.empty(), L"File should contain written data");
        }
        TEST_METHOD(TestCreateRandomValues)
        {
            PositionData::Position pos;
            pos.createRandomValues();

            // Verify ranges for each attribute
            Assert::IsTrue(pos.latitude >= -90.0 && pos.latitude <= 90.0,
                L"Latitude should be between -90 and 90");
            Assert::IsTrue(pos.longitude >= -180.0 && pos.longitude <= 180.0,
                L"Longitude should be between -180 and 180");
            Assert::IsTrue(pos.heading >= 0.0 && pos.heading <= 360.0,
                L"Heading should be between 0 and 360");
            Assert::IsTrue(pos.velocity >= 250.0 && pos.velocity <= 600.0,
                L"Velocity should be between 250 and 600");
            Assert::IsTrue(pos.altitude >= 1000.0 && pos.altitude <= 10000.0,
                L"Altitude should be between 1000 and 10000");
        }
        TEST_METHOD(TestSerialize)
        {
            // Prepare test position
            PositionData::Position original(40.7128, -74.0060, 90.0, 500.0, 5000.0);

            // Create buffer for serialization
            char buffer[POSITION_SIZE];

            // Serialize
            int serializeResult = original.Serialize(buffer);

            // Verify serialization succeeded
            Assert::AreEqual(static_cast<int>(PositionData::Position::GetSerializedSize()),
                serializeResult, L"Serialization should return correct size");

            // Deserialize
            PositionData::Position deserialized(buffer);

            // Compare original and deserialized
            Assert::AreEqual(original.latitude, deserialized.latitude,
                L"Latitude not correctly serialized/deserialized");
            Assert::AreEqual(original.longitude, deserialized.longitude,
                L"Longitude not correctly serialized/deserialized");
            Assert::AreEqual(original.heading, deserialized.heading,
                L"Heading not correctly serialized/deserialized");
            Assert::AreEqual(original.velocity, deserialized.velocity,
                L"Velocity not correctly serialized/deserialized");
            Assert::AreEqual(original.altitude, deserialized.altitude,
                L"Altitude not correctly serialized/deserialized");
        }
        TEST_METHOD(TestSerialize_NullBuffer)
        {
            PositionData::Position pos(40.7128, -74.0060, 90.0, 500.0, 5000.0);

            // Attempt to serialize to null buffer
            int serializeResult = pos.Serialize(nullptr);

            // Verify error return
            Assert::AreEqual(pos.ERR, serializeResult, L"Serialization to null buffer should return error");
        }
        TEST_METHOD(TestStaticMethods)
        {
            // Verify attribute count
            Assert::AreEqual((unsigned int)POSITION_ATTR_COUNT, PositionData::Position::GetAttributeCount(),
                L"Attribute count should be 5");

            // Verify serialized size
            Assert::AreEqual(POSITION_SIZE, PositionData::Position::GetSerializedSize(), L"Serialized size should be 40");
        }
    };
}