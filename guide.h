#ifndef GUIDE_H
#define GUIDE_H
#include <string>
using namespace std;

class Guide {
public:
    int guideId;
    string name;
    string licenseNumber;
    string languages;
    int experienceYears;
    double rating;
    string contact;
    double dailyRate; // NPR charged per trekking day when this guide is assigned to a booking

    Guide() {}
    Guide(int id, string n, string lic, string lang, int exp, double r, string c, double rate)
        : guideId(id), name(n), licenseNumber(lic), languages(lang),
        experienceYears(exp), rating(r), contact(c), dailyRate(rate) {}
};

#endif // GUIDE_H