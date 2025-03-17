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

    // Getters and Setters
    double getLatitude() const { return latitude; }
    double getLongitude() const { return longitude; }
    double getHeading() const { return heading; }
    double getVelocity() const { return velocity; }
    double getAltitude() const { return altitude; }
    void setLatitude(double lat) { latitude = lat; }
    void setLongitude(double lon) { longitude = lon; }
    void setHeading(double hdg) { heading = hdg; }
    void setVelocity(double vel) { velocity = vel; }
    void setAltitude(double alt) { altitude = alt; }

    // File IO Functions
    bool writeToFile(std::string filePath) {

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
    std::string createStringToSend() {
        return std::to_string(latitude) + "," + std::to_string(longitude) + "," + std::to_string(heading) + "," + std::to_string(velocity) + "," + std::to_string(altitude);
    }
};