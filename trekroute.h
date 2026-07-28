#ifndef TREKROUTE_H
#define TREKROUTE_H

#include <string>
#include <vector>
using namespace std;

class TrekRoute {
public:
    int trekId;
    string name;
    int daysRequired;
    string difficulty;    // "Beginner", "Intermediate", "Expert"
    double maxAltitude;
    double estimatedCost;
    string bestSeason;
    bool permitRequired;
    vector<string> checkpoints;

    TrekRoute() {}

    TrekRoute(int id, string n, int days, string diff,
              double alt, double cost, string season,
              bool permit, vector<string> cp)
        : trekId(id), name(n), daysRequired(days), difficulty(diff),
        maxAltitude(alt), estimatedCost(cost), bestSeason(season),
        permitRequired(permit), checkpoints(cp) {}
};

#endif // TREKROUTE_H
