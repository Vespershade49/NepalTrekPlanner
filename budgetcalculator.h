#ifndef BUDGETCALCULATOR_H
#define BUDGETCALCULATOR_H

#include "touristspot.h"
#include "trekroute.h"
#include <vector>
#include <string>
using namespace std;

class BudgetCalculator {
private:
    // Approximate NPR to USD conversion rate
    const double NPR_TO_USD = 133.0;

public:
    // Accommodation cost per night in NPR
    double getAccommodationRate(string type) {
        if (type == "budget")   return 1500.0;
        if (type == "standard") return 3000.0;
        if (type == "luxury")   return 6000.0;
        return 1500.0; // default to budget
    }

    // Calculate total trip cost in NPR
    double calculate(TrekRoute route,
                     vector<TouristSpot> spots,
                     int days,
                     string accommodationType,
                     int numberOfPeople) {

        double total = 0.0;

        // Trek base cost
        total += route.estimatedCost;

        // Entry fees for all selected spots
        for (TouristSpot spot : spots)
            total += spot.entryFee;

        // Accommodation: rate * days * people
        double accomRate = getAccommodationRate(accommodationType);
        total += accomRate * days * numberOfPeople;

        // Food estimate: 500 NPR per person per day
        total += 500.0 * days * numberOfPeople;

        // Transport estimate: fixed base from Kathmandu
        total += 2000.0 * numberOfPeople;

        return total;
    }

    // Convert NPR to USD
    double toUSD(double npr) {
        return npr / NPR_TO_USD;
    }

    // Get breakdown as individual components (useful for Qt display)
    struct CostBreakdown {
        double trekCost;
        double entryFees;
        double accommodation;
        double food;
        double transport;
        double totalNPR;
        double totalUSD;
    };

    CostBreakdown getBreakdown(TrekRoute route,
                               vector<TouristSpot> spots,
                               int days,
                               string accommodationType,
                               int numberOfPeople) {
        CostBreakdown b;
        b.trekCost      = route.estimatedCost;
        b.entryFees     = 0;
        for (TouristSpot spot : spots)
            b.entryFees += spot.entryFee;
        b.accommodation = getAccommodationRate(accommodationType) * days * numberOfPeople;
        b.food          = 500.0 * days * numberOfPeople;
        b.transport     = 2000.0 * numberOfPeople;
        b.totalNPR      = b.trekCost + b.entryFees + b.accommodation + b.food + b.transport;
        b.totalUSD      = toUSD(b.totalNPR);
        return b;
    }
};

#endif // BUDGETCALCULATOR_H
