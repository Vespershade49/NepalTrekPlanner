#ifndef TOURISTSPOT_H
#define TOURISTSPOT_H
#include <string>
#include <vector>
using namespace std;

class TouristSpot {
public:
    int spotId;
    string name;
    string location;
    string category;
    string bestTime;
    double entryFee;
    string description;

    TouristSpot() {}

    TouristSpot(int id, string n, string loc, string cat,
                string best, double fee, string desc)
        : spotId(id), name(n), location(loc), category(cat),
        bestTime(best), entryFee(fee), description(desc) {}
};

#endif // TOURISTSPOT_H