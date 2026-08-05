#ifndef BACKEND_H
#define BACKEND_H
#include "touristspot.h"
#include "trekroute.h"
#include "emergencyinfo.h"
#include "budgetcalculator.h"
#include "filemanager.h"          // for saving/loading itineraries to a text file
#include "itinerary.h"
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <QString>
using namespace std;

class Backend {
public:
    vector<TouristSpot>               touristSpots;
    vector<TrekRoute>                 trekRoutes;
    map<string, vector<TouristSpot>>  spotsByCategory;
    map<string, EmergencyInfo>        emergencyByRegion;
    BudgetCalculator                  budgetCalc;
    FileManager                       fileManager;      // handles saving/loading itineraries
    vector<Itinerary>                 itineraries;
    int                                nextItineraryId = 1;

    // ── Add data ──────────────────────────────────────────────
    void addTouristSpot(TouristSpot spot) {
        touristSpots.push_back(spot);
        spotsByCategory[spot.category].push_back(spot);
    }

    void addTrekRoute(TrekRoute route) {
        trekRoutes.push_back(route);
    }

    void addEmergencyInfo(EmergencyInfo info) {
        emergencyByRegion[info.region] = info;
    }

    // ── Search & Filter ───────────────────────────────────────
    // Search spots by name keyword (case-insensitive)
    vector<TouristSpot> searchSpotsByName(string keyword) {
        vector<TouristSpot> results;
        string kw = keyword;
        transform(kw.begin(), kw.end(), kw.begin(), ::tolower);

        for (TouristSpot spot : touristSpots) {
            string name = spot.name;
            transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.find(kw) != string::npos)
                results.push_back(spot);
        }
        return results;
    }

    // Get all spots in a category
    vector<TouristSpot> getSpotsByCategory(string category) {
        return spotsByCategory[category];
    }

    // ── Emergency Info ────────────────────────────────────────
    EmergencyInfo getEmergencyInfo(string region) {
        if (emergencyByRegion.count(region))
            return emergencyByRegion[region];
        return EmergencyInfo(); // empty if not found
    }

    // Get all emergency regions available
    vector<string> getAllEmergencyRegions() {
        vector<string> regions;
        for (auto& pair : emergencyByRegion)
            regions.push_back(pair.first);
        return regions;
    }

    // ── Budget Helper ─────────────────────────────────────────
    BudgetCalculator::CostBreakdown calculateBudget(
        TrekRoute route,
        int days,
        string accommodationType,
        int numberOfPeople)
    {
        return budgetCalc.getBreakdown(route, days,
                                       accommodationType, numberOfPeople);
    }
    bool saveItinerary(string itineraryText, string timestamp) {
        return fileManager.saveItinerary(itineraryText, timestamp);
    }

    string loadItineraries() {
        return fileManager.loadItineraries();
    }

    // ── Itinerary Creation (NEW) ───────────────────────────────
    // "Itinerary contains one TrekRoute and one-or-more TouristSpots,
    //  resolved through Transaction."
    Itinerary createItinerary(
        int userId,
        TrekRoute trek,
        vector<TouristSpot> spots,
        double totalCost,
        string createdDate)
    {
        Itinerary itinerary(nextItineraryId, userId, totalCost, createdDate);
        nextItineraryId++;

        // One Transaction linking the itinerary to its trek
        itinerary.addTransaction(trek.trekId, "trek");

        // One Transaction per tourist spot the user picked
        for (const TouristSpot &spot : spots)
            itinerary.addTransaction(spot.spotId, "spot");

        itineraries.push_back(itinerary);
        return itinerary;
    }
};

#endif