#include <fstream>
#include <string>
#include <sstream>
#include <random>

class Position {
private:
    static constexpr unsigned int ATTRIBUTE_COUNT = 5;
    double latitude = 0.0;
    double longitude = 0.0;
    double heading = 0.0;
    double velocity = 0.0; // Knots
    double altitude = 0.0; // Metres

public:
    // Constructors
    Position() = default;
    Position(double lat, double lon, double hdg, double vel, double alt)
        : latitude(lat), longitude(lon), heading(hdg), velocity(vel), altitude(alt) {
    }
    Position(const std::string& csvData) {
        int parsed = sscanf_s(csvData.c_str(), "%lf,%lf,%lf,%lf,%lf",
            &latitude, &longitude, &heading, &velocity, &altitude);

        if (parsed != ATTRIBUTE_COUNT) {
            latitude = longitude = heading = velocity = altitude = 0.0;
        }
    }

    // File IO Functions
    bool writeToFile(std::string filePath, std::string longitude, std::string latitude, std::string heading, std::string velocity, std::string altitude ) {

        std::ofstream file;
        file.open(filePath, std::ios::app);
        if (file.is_open()) {
            file << latitude << "," << longitude << "," << heading << "," << velocity << "," << altitude << std::endl;
            file.close();
            return true;
        }
        else {
            return false;
        }
    }

    // Create Random Values
    void createRandomValues() {
        //std::random_device rd;
        std::mt19937 gen(time(nullptr));

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
    int Serialize(char* outBuff) {

        int offset = 0;

        memcpy(outBuff + offset, &latitude, sizeof(double));
        offset += sizeof(double);
        memcpy(outBuff + offset, &longitude, sizeof(double));
        offset += sizeof(double);
        memcpy(outBuff + offset, &heading, sizeof(double));
        offset += sizeof(double);
        memcpy(outBuff + offset, &velocity, sizeof(double));
        offset += sizeof(double);
        memcpy(outBuff + offset, &altitude, sizeof(double));

        return (sizeof(double) * ATTRIBUTE_COUNT);
    }

	static unsigned int GetAttributeCount() {
		return ATTRIBUTE_COUNT;
	}

	static unsigned int GetSerializedSize() {
		return sizeof(double) * ATTRIBUTE_COUNT;
	}
};