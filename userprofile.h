#ifndef USERPROFILE_H
#define USERPROFILE_H

#include <string>
using namespace std;

class UserProfile {
public:
    int userId;
    string name;
    string fitnessLevel;  // "Beginner", "Intermediate", "Expert"
    int availableDays;
    double budget;        // in NPR

    UserProfile() {}

    UserProfile(int id, string n, string fitness, int days, double b)
        : userId(id), name(n), fitnessLevel(fitness),
        availableDays(days), budget(b) {}
};

#endif // USERPROFILE_H
