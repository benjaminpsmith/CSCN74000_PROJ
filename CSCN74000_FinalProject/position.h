#include <fstream>
#include <string>
#include <random>

class Position {
private:
    double latitude;
    double longitude;
    double heading;
    double velocity; // Knots
    double altitude;

public:
    // Constructors
	Position() : latitude(0), longitude(0), heading(0), velocity(0), altitude(0) {
    }
    Position(double lat, double lon, double hdg, double vel, double alt)
        : latitude(lat), longitude(lon), heading(hdg), velocity(vel), altitude(alt) {
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
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<double> lat(-90.0, 90.0);
        std::uniform_real_distribution<double> lon(-180.0, 180.0);
        std::uniform_real_distribution<double> head(0.0, 360.0);
        std::uniform_real_distribution<double> vel(250.0, 600.0);
        std::uniform_real_distribution<double> alt(0.0, 1000.0);

        latitude = lat(gen);
        longitude = lon(gen);
        heading = head(gen);
        velocity = vel(gen);
        altitude = alt(gen);
	}
};