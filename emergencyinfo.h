#ifndef EMERGENCYINFO_H
#define EMERGENCYINFO_H

#include <string>
using namespace std;

class EmergencyInfo {
public:
    string region;
    string contactName;
    string phoneNumber;
    string hospitalName;
    string altitudeSicknessWarning;

    EmergencyInfo() {}

    EmergencyInfo(string reg, string contact, string phone,
                  string hospital, string warning)
        : region(reg), contactName(contact), phoneNumber(phone),
        hospitalName(hospital), altitudeSicknessWarning(warning) {}
};

#endif // EMERGENCYINFO_H
