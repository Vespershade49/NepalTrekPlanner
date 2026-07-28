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

    Guide() {}
    Guide(int id, string n, string lic, string lang, int exp, double r, string c)
        : guideId(id), name(n), licenseNumber(lic), languages(lang),
        experienceYears(exp), rating(r), contact(c) {}
};

#endif // GUIDE_H