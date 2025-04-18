#include <fstream>
#include <string>
#include <sstream>
#include <random>
#include <cstring>
#include <stdexcept>



namespace PositionData {

    class Position {
#
    public:
        static constexpr unsigned int ATTRIBUTE_COUNT = 5;
        double latitude = 0.0;
        double longitude = 0.0;
        double heading = 0.0;
        double velocity = 0.0; // Knots
        double altitude = 0.0; // Metres

        const int ERR = -1;
        const std::string error_msg = "An error has occured.";

    public:
        // Constructors
        Position() = default;
        Position(double lat, double lon, double hdg, double vel, double alt)
            : latitude(lat), longitude(lon), heading(hdg), velocity(vel), altitude(alt) {
        }
        Position(const char rawData[]) {
            if (rawData == nullptr) {
                throw std::invalid_argument(error_msg);
            }

            size_t offset = 0;

            // Deserialize latitude
            if (std::memcpy(&latitude, &rawData[offset], sizeof(double)) != &latitude) {
                throw std::runtime_error(error_msg);
            }
            offset += sizeof(double);

            // Deserialize longitude
            if (std::memcpy(&longitude, &rawData[offset], sizeof(double)) != &longitude) {
                throw std::runtime_error(error_msg);
            }
            offset += sizeof(double);

            // Deserialize heading
            if (std::memcpy(&heading, &rawData[offset], sizeof(double)) != &heading) {
                throw std::runtime_error(error_msg);
            }
            offset += sizeof(double);

            // Deserialize velocity
            if (std::memcpy(&velocity, &rawData[offset], sizeof(double)) != &velocity) {
                throw std::runtime_error(error_msg);
            }
            offset += sizeof(double);

            // Deserialize altitude
            if (std::memcpy(&altitude, &rawData[offset], sizeof(double)) != &altitude) {
                throw std::runtime_error(error_msg);
            }
        }

        // File IO Functions
        bool writeToFile(const std::string& filePath) {

            bool success = true;

            std::ofstream file;
            file.open(filePath, std::ios::app);
            if (file.is_open()) {
                std::ostringstream oss;
                oss << latitude << "," << longitude << "," << heading << "," << velocity << "," << altitude;
                file << oss.str() << std::endl;
                file.close();
            }
            else {
                success = false;
            }

            return success;
        }

        // Create Random Values
        void createRandomValues() {
            std::random_device rd;
            std::mt19937 gen(rd());

            std::uniform_real_distribution<double> lat(-90.0, 90.0);
            std::uniform_real_distribution<double> lon(-180.0, 180.0);
            std::uniform_real_distribution<double> head(0.0, 360.0);
            std::uniform_real_distribution<double> vel(250.0, 600.0);
            std::uniform_real_distribution<double> alt(1000.0, 10000.0);

            latitude = lat(gen);
            longitude = lon(gen);
            heading = head(gen);
            velocity = vel(gen);
            altitude = alt(gen);
        }

        // Create a string that we can used to build a packet body and send.
        int Serialize(char outBuff[]) {

            int retValue = ERR;

            if (outBuff != nullptr) {

                bool success = true;

                size_t offset = 0;

                // Serialize latitude 
                if (memcpy(&outBuff[offset], &latitude, sizeof(double)) != &outBuff[offset]) { success = false; }
                offset += sizeof(double);

                // Serialize longitude
                if (memcpy(&outBuff[offset], &longitude, sizeof(double)) != &outBuff[offset]) { success = false; }
                offset += sizeof(double);

                // Serialize heading
                if (memcpy(&outBuff[offset], &heading, sizeof(double)) != &outBuff[offset]) { success = false; }
                offset += sizeof(double);

                // Serialize velocity
                if (memcpy(&outBuff[offset], &velocity, sizeof(double)) != &outBuff[offset]) { success = false; }
                offset += sizeof(double);

                // Serialize altitude
                if (memcpy(&outBuff[offset], &altitude, sizeof(double)) != &outBuff[offset]) { success = false; }

                if (success) {
                    retValue = sizeof(double) * ATTRIBUTE_COUNT;
                }
            }
            else {
                retValue = ERR;
            }

            return retValue;
        }

        static unsigned int GetAttributeCount() {
            return ATTRIBUTE_COUNT;
        }

        static int GetSerializedSize() {
            return sizeof(double) * ATTRIBUTE_COUNT;
        }
    };
}