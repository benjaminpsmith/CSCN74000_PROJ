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
    TEST_CLASS(PacketDefConstructionTests)
    {
    public:
        TEST_METHOD(DefaultConstructor_InitializesAllFieldsToZero)
        {
            PacketData::PacketDef packet;

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

        TEST_METHOD(ParameterizedConstructor_SetsAllFieldsCorrectly)
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

        TEST_METHOD(RawDataConstructor_DeserializesPacketCorrectly)
        {
            uint32_t src = 0xA0B1C2;
            uint32_t dest = 0xD3E4F5;
            PacketData::PacketDef::Flag flag = PacketData::PacketDef::Flag::BB;
            unsigned int seqNum = 1;
            unsigned int totalCount = 10;
            unsigned int bodyLen = 20;
            unsigned int crc = 0x12345678;

            char buffer[PacketData::Constants::MAX_PACKET_LENGTH];
            size_t offset = 0;

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

            char bodyData[20];
            std::memset(bodyData, 0xAA, bodyLen);
            std::memcpy(&buffer[offset], bodyData, bodyLen);
            offset += bodyLen;

            std::memcpy(&buffer[offset], &crc, sizeof(crc));

            PacketData::PacketDef packet(buffer, PacketData::Constants::MAX_PACKET_LENGTH);

            Assert::AreEqual(src, packet.getSrc(), L"Source not deserialized correctly");
            Assert::AreEqual(dest, packet.getDest(), L"Destination not deserialized correctly");
            Assert::AreEqual(flag, packet.getFlag(), L"Flag not deserialized correctly");
            Assert::AreEqual(seqNum, packet.getSeqNum(), L"Sequence number not deserialized correctly");
            Assert::AreEqual(totalCount, packet.getTotalCount(), L"Total count not deserialized correctly");
            Assert::AreEqual(bodyLen, packet.getBodyLen(), L"Body length not deserialized correctly");
            Assert::AreEqual(crc, packet.getCrc(), L"CRC not deserialized correctly");
        }

        TEST_METHOD(RawDataConstructor_ThrowsOnNullData)
        {
            auto func = [] {
                PacketData::PacketDef packet(nullptr, 50);
                };
            Assert::ExpectException<std::invalid_argument>(func,
                L"Null data should throw invalid_argument");
        }

        TEST_METHOD(RawDataConstructor_ThrowsOnBufferTooSmall)
        {
            char buffer[PacketData::Constants::MIN_PACKET_LENGTH - 1];
            auto func = [&buffer] {
                PacketData::PacketDef packet(buffer, PacketData::Constants::MIN_PACKET_LENGTH - 1);
                };
            Assert::ExpectException<std::invalid_argument>(func,
                L"Too small buffer should throw invalid_argument");
        }
    };

    TEST_CLASS(PacketDefSetterGetterTests)
    {
    public:
        TEST_METHOD(SetSrc_UpdatesSourceCorrectly)
        {
            PacketData::PacketDef packet;
            packet.setSrc(0xA0B1C2);
            Assert::AreEqual(0xA0B1C2u, packet.getSrc(), L"Source not set correctly");
        }

        TEST_METHOD(SetDest_UpdatesDestinationCorrectly)
        {
            PacketData::PacketDef packet;
            packet.setDest(0xD3E4F5);
            Assert::AreEqual(0xD3E4F5u, packet.getDest(), L"Destination not set correctly");
        }

        TEST_METHOD(SetFlag_UpdatesFlagCorrectly)
        {
            PacketData::PacketDef packet;
            packet.setFlag(PacketData::PacketDef::Flag::IMG);
            Assert::AreEqual(PacketData::PacketDef::Flag::IMG, packet.getFlag(),
                L"Flag not set correctly");
        }

        TEST_METHOD(SetSeqNum_UpdatesSequenceNumberCorrectly)
        {
            PacketData::PacketDef packet;
            packet.setSeqNum(5);
            Assert::AreEqual(5u, packet.getSeqNum(), L"Sequence number not set correctly");
        }

        TEST_METHOD(SetTotalCount_UpdatesTotalCountCorrectly)
        {
            PacketData::PacketDef packet;
            packet.setTotalCount(20);
            Assert::AreEqual(20u, packet.getTotalCount(), L"Total count not set correctly");
        }

        TEST_METHOD(SetCrc_UpdatesCrcCorrectly)
        {
            PacketData::PacketDef packet;
            packet.setCrc(0x12345678);
            Assert::AreEqual(0x12345678u, packet.getCrc(), L"CRC not set correctly");
        }
    };

    TEST_CLASS(PacketDefDataTests)
    {
    public:
        TEST_METHOD(SetData_CopiesDataCorrectly)
        {
            PacketData::PacketDef packet;
            char testData[100];
            std::memset(testData, 0xAA, sizeof(testData));

            int result = packet.setData(testData, sizeof(testData));

            Assert::AreEqual(SUCCESS_CODE, result, L"Data setting should succeed");
            Assert::AreEqual(sizeof(testData), static_cast<size_t>(packet.getBodyLen()),
                L"Body length not set correctly");
            Assert::IsNotNull(packet.getData(), L"Data should not be null");
            Assert::IsTrue(std::memcmp(testData, packet.getData(), sizeof(testData)) == 0,
                L"Data not copied correctly");
        }

        TEST_METHOD(SetData_ReturnsErrorForOversizedData)
        {
            PacketData::PacketDef packet;
            char largeData[PacketData::Constants::MAX_BODY_LENGTH + 1];

            int result = packet.setData(largeData, sizeof(largeData));

            Assert::AreEqual(ERROR_CODE, result, L"Oversized data should fail");
        }
    };

    TEST_CLASS(PacketDefSerializationTests)
    {
    public:
        TEST_METHOD(Serialize_ProducesValidOutput)
        {
            PacketData::PacketDef packet(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            PositionData::Position pos(12.34, 56.78, 90.0, 450.0, 5000.0);
            char bodyData[POSITION_SIZE];
            pos.Serialize(bodyData);

            packet.setData(bodyData, PositionData::Position::GetSerializedSize());
            packet.setCrc(0x12345678);

            char buffer[PacketData::Constants::MAX_PACKET_LENGTH];
            int serializeResult = packet.Serialize(buffer);

            Assert::IsTrue(serializeResult > 0, L"Serialization should succeed");
            Assert::IsTrue(serializeResult <= PacketData::Constants::MAX_PACKET_LENGTH,
                L"Serialized packet length should be within max length");
        }

        TEST_METHOD(Serialize_Deserialize_RoundTripWorks)
        {
            PacketData::PacketDef originalPacket(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            PositionData::Position pos(12.34, 56.78, 90.0, 450.0, 5000.0);
            char bodyData[POSITION_SIZE];
            pos.Serialize(bodyData);

            originalPacket.setData(bodyData, PositionData::Position::GetSerializedSize());
            originalPacket.setCrc(0x12345678);

            char buffer[PacketData::Constants::MAX_PACKET_LENGTH];
            originalPacket.Serialize(buffer);

            PacketData::PacketDef deserializedPacket(buffer, PacketData::Constants::MAX_PACKET_LENGTH);

            Assert::AreEqual(originalPacket.getSrc(), deserializedPacket.getSrc(),
                L"Source not correctly serialized/deserialized");
            Assert::AreEqual(originalPacket.getCrc(), deserializedPacket.getCrc(),
                L"CRC not correctly serialized/deserialized");
        }

        TEST_METHOD(Serialize_ReturnsErrorForNullBuffer)
        {
            PacketData::PacketDef packet(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            int serializeResult = packet.Serialize(nullptr);

            Assert::AreEqual(ERROR_CODE, serializeResult,
                L"Serialization to null buffer should return error");
        }
    };

    TEST_CLASS(PacketDefAssignmentTests)
    {
    public:
        TEST_METHOD(AssignmentOperator_CopiesAllFieldsCorrectly)
        {
            PacketData::PacketDef packet1(0xA0B1C2, 0xD3E4F5,
                PacketData::PacketDef::Flag::BB, 1, 10);

            PositionData::Position pos(12.34, 56.78, 90.0, 450.0, 5000.0);
            char bodyData[POSITION_SIZE];
            pos.Serialize(bodyData);

            packet1.setData(bodyData, PositionData::Position::GetSerializedSize());
            packet1.setCrc(0x12345678);

            PacketData::PacketDef packet2;
            int assignResult = (packet2 = packet1);

            Assert::AreEqual(SUCCESS_CODE, assignResult, L"Assignment should succeed");
            Assert::AreEqual(packet1.getSrc(), packet2.getSrc(), L"Source not copied");
            Assert::AreEqual(packet1.getDest(), packet2.getDest(), L"Destination not copied");
            Assert::AreEqual(packet1.getFlag(), packet2.getFlag(), L"Flag not copied");
            Assert::AreEqual(packet1.getSeqNum(), packet2.getSeqNum(), L"Sequence number not copied");
            Assert::AreEqual(packet1.getTotalCount(), packet2.getTotalCount(), L"Total count not copied");
            Assert::AreEqual(packet1.getBodyLen(), packet2.getBodyLen(), L"Body length not copied");
            Assert::AreEqual(packet1.getCrc(), packet2.getCrc(), L"CRC not copied");
            Assert::IsTrue(std::memcmp(packet1.getData(), packet2.getData(),
                packet1.getBodyLen()) == 0, L"Body data not copied correctly");
        }
    };
}

namespace PositionDataTests
{
    TEST_CLASS(PositionConstructionTests)
    {
    public:
        TEST_METHOD(DefaultConstructor_InitializesAllFieldsToZero)
        {
            PositionData::Position pos;

            Assert::AreEqual(0.0, pos.latitude, L"Latitude should be 0.0");
            Assert::AreEqual(0.0, pos.longitude, L"Longitude should be 0.0");
            Assert::AreEqual(0.0, pos.heading, L"Heading should be 0.0");
            Assert::AreEqual(0.0, pos.velocity, L"Velocity should be 0.0");
            Assert::AreEqual(0.0, pos.altitude, L"Altitude should be 0.0");
        }

        TEST_METHOD(ParameterizedConstructor_SetsAllFieldsCorrectly)
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

        TEST_METHOD(RawDataConstructor_DeserializesDataCorrectly)
        {
            double testData[POSITION_ATTR_COUNT] = { 40.7128, -74.0060, 90.0, 500.0, 5000.0 };
            char rawData[POSITION_SIZE];
            std::memcpy(rawData, testData, sizeof(rawData));

            PositionData::Position pos(rawData);

            Assert::AreEqual(testData[0], pos.latitude, L"Latitude not deserialized correctly");
            Assert::AreEqual(testData[1], pos.longitude, L"Longitude not deserialized correctly");
            Assert::AreEqual(testData[2], pos.heading, L"Heading not deserialized correctly");
            Assert::AreEqual(testData[3], pos.velocity, L"Velocity not deserialized correctly");
            Assert::AreEqual(testData[4], pos.altitude, L"Altitude not deserialized correctly");
        }

        TEST_METHOD(RawDataConstructor_ThrowsOnNullPointer)
        {
            auto testFunc = [] {
                PositionData::Position pos(nullptr);
                };

            Assert::ExpectException<std::invalid_argument>(testFunc,
                L"Null pointer should throw invalid_argument");
        }
    };

    TEST_CLASS(PositionFileIOTests)
    {
    public:
        TEST_METHOD(WriteToFile_CreatesFileWithData)
        {
            PositionData::Position pos(40.7128, -74.0060, 90.0, 500.0, 5000.0);
            std::string testFilePath = "test_position_output.txt";

            if (std::filesystem::exists(testFilePath)) {
                std::filesystem::remove(testFilePath);
            }

            bool writeResult = pos.writeToFile(testFilePath);

            Assert::IsTrue(writeResult, L"File write should succeed");
            Assert::IsTrue(std::filesystem::exists(testFilePath), L"File should be created");

            std::ifstream file(testFilePath);
            std::string line;
            std::getline(file, line);
            file.close();
            std::filesystem::remove(testFilePath);

            Assert::IsFalse(line.empty(), L"File should contain written data");
        }
    };

    TEST_CLASS(PositionRandomizationTests)
    {
    public:
        TEST_METHOD(CreateRandomValues_GeneratesValuesInExpectedRanges)
        {
            PositionData::Position pos;
            pos.createRandomValues();

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
    };

    TEST_CLASS(PositionSerializationTests)
    {
    public:
        TEST_METHOD(Serialize_ProducesValidOutput)
        {
            PositionData::Position original(40.7128, -74.0060, 90.0, 500.0, 5000.0);
            char buffer[POSITION_SIZE];

            int serializeResult = original.Serialize(buffer);

            Assert::AreEqual(static_cast<int>(PositionData::Position::GetSerializedSize()),
                serializeResult, L"Serialization should return correct size");
        }

        TEST_METHOD(Serialize_Deserialize_RoundTripWorks)
        {
            PositionData::Position original(40.7128, -74.0060, 90.0, 500.0, 5000.0);
            char buffer[POSITION_SIZE];
            original.Serialize(buffer);

            PositionData::Position deserialized(buffer);

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

        TEST_METHOD(Serialize_ReturnsErrorForNullBuffer)
        {
            PositionData::Position pos(40.7128, -74.0060, 90.0, 500.0, 5000.0);
            int serializeResult = pos.Serialize(nullptr);

            Assert::AreEqual(ERROR_CODE, serializeResult,
                L"Serialization to null buffer should return error");
        }
    };

    TEST_CLASS(PositionStaticMethodTests)
    {
    public:
        TEST_METHOD(GetAttributeCount_ReturnsCorrectValue)
        {
            Assert::AreEqual((unsigned int)POSITION_ATTR_COUNT,
                PositionData::Position::GetAttributeCount(),
                L"Attribute count should be 5");
        }

        TEST_METHOD(GetSerializedSize_ReturnsCorrectValue)
        {
            Assert::AreEqual(POSITION_SIZE,
                PositionData::Position::GetSerializedSize(),
                L"Serialized size should be 40");
        }
    };
}