#ifndef BUDGETCALCULATOR_H
#define BUDGETCALCULATOR_H

#include "trekroute.h"
#include <vector>
#include <string>
using namespace std;

// NOTE: tourist-spot entry fees used to be part of this calculation
// (selected spots -> per-person ticket costs). That selection step has
// been removed from the Budget Calculator UI, so BudgetCalculator no
// longer takes or charges for a spots list at all - trip cost is now
// just trek + accommodation + food + transport.
class BudgetCalculator {
private:
    // Approximate NPR to USD conversion rate
    const double NPR_TO_USD = 133.0;

    // route.estimatedCost is quoted for the trek's *standard* duration
    // (route.daysRequired) - e.g. a 14-day EBC package. If the trekker
    // picks a different number of days in the Budget Calculator, the
    // trek cost should scale proportionally, the same way a real
    // agency's guide/permit fee scales with trip length rather than
    // staying flat no matter how long you stay.
    //
    // It also scales per person: guides, porters, and permits in real
    // trekking packages are quoted per trekker, not once for the whole
    // group - a group of 4 pays roughly 4x what 1 person pays, not the
    // same flat amount.
    double calculateTrekCost(const TrekRoute &route, int days, int numberOfPeople) {
        double dailyRatePerPerson = (route.daysRequired > 0)
        ? route.estimatedCost / route.daysRequired
        : route.estimatedCost;
        return dailyRatePerPerson * days * numberOfPeople;
    }

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
                     int days,
                     string accommodationType,
                     int numberOfPeople) {

        double total = 0.0;

        // Trek base cost - now scaled per day and per person (see
        // calculateTrekCost above), not a single flat number regardless
        // of trip length or group size.
        total += calculateTrekCost(route, days, numberOfPeople);

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
        double accommodation;
        double food;
        double transport;
        double totalNPR;
        double totalUSD;
    };

    CostBreakdown getBreakdown(TrekRoute route,
                               int days,
                               string accommodationType,
                               int numberOfPeople) {
        CostBreakdown b;
        b.trekCost      = calculateTrekCost(route, days, numberOfPeople);
        b.accommodation = getAccommodationRate(accommodationType) * days * numberOfPeople;
        b.food          = 500.0 * days * numberOfPeople;
        b.transport     = 2000.0 * numberOfPeople;
        b.totalNPR      = b.trekCost + b.accommodation + b.food + b.transport;
        b.totalUSD      = toUSD(b.totalNPR);
        return b;
    }
};

#endif // BUDGETCALCULATOR_H