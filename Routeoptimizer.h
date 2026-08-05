#ifndef ROUTEOPTIMIZER_H
#define ROUTEOPTIMIZER_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <limits>
#include <algorithm>
using namespace std;

// Represents a road/path between two cities
struct Edge {
    string destination;
    double cost;      // transport cost in NPR
    double distance;  // in km
};

// One point along a computed route where the ascent between this
// checkpoint and the previous one exceeds the safe daily limit.
// This is what actually backs the report's AMS/altitude-safety claim -
// before this, RouteOptimizer had no altitude data at all.
struct AmsWarning {
    string fromCity;
    string toCity;
    double ascentMeters;   // how much higher toCity is than fromCity
};

// Result of Dijkstra
struct RouteResult {
    vector<string> path;       // cities in order
    double totalCost;          // total transport cost in NPR
    double totalDistance;      // total km
    bool found;                // false if no path exists (or none fits the budget)
    bool budgetLimited;        // true if 'found' is false specifically because
    // every reachable path exceeded maxBudget
    vector<AmsWarning> amsWarnings; // any single-segment ascent over the safe limit
};

class RouteOptimizer {
private:
    // Adjacency list: city -> list of connected cities with cost
    map<string, vector<Edge>> graph;

    // Approximate altitude in meters for each named checkpoint. This is
    // what lets the engine reason about Acute Mountain Sickness (AMS)
    // risk along a route, instead of only optimizing for cost/distance.
    // The Himalayan Rescue Association's general guidance is to keep
    // sleeping-altitude gain to roughly 500m per day above ~3000m -
    // that threshold is what AMS_SAFE_ASCENT_METERS below enforces.
    map<string, double> altitude;

public:
    // Safe daily ascent limit in meters, per HRA acclimatization
    // guidance. Exceeding this between two consecutive checkpoints on
    // a route triggers an AmsWarning rather than failing the route -
    // trekkers can still choose to go, but they should know.
    static constexpr double AMS_SAFE_ASCENT_METERS = 500.0;

    RouteOptimizer() {
        buildNepalGraph();
        buildAltitudeTable();
    }

    // Build the graph of Nepal cities with real approximate costs
    void buildNepalGraph() {

        // Format: addRoute(cityA, cityB, costNPR, distanceKm)
        // Roads are bidirectional

        // Kathmandu connections
        addRoute("Kathmandu",  "Pokhara",       2500,  200);
        addRoute("Kathmandu",  "Chitwan",        1500,  150);
        addRoute("Kathmandu",  "Lukla",          8000,  140); // flight
        addRoute("Kathmandu",  "Syabrubesi",     1200,  120); // Langtang start
        addRoute("Kathmandu",  "Besisahar",      1500,  175); // Annapurna Circuit start
        addRoute("Kathmandu",  "Lumbini",        2000,  280);
        addRoute("Kathmandu",  "Jomsom",         9000,  270); // Mustang (flight)

        // Pokhara connections
        addRoute("Pokhara",    "Nayapul",         500,   42); // Poon Hill start
        addRoute("Pokhara",    "Besisahar",       800,   80);
        addRoute("Pokhara",    "Chitwan",        1200,  150);
        addRoute("Pokhara",    "Jomsom",         5000,  160); // flight or road
        addRoute("Pokhara",    "Lumbini",        1200,  200);

        // Trek starting points
        addRoute("Lukla",      "Namche Bazaar",  1000,   65);
        addRoute("Namche Bazaar", "Tengboche",    500,   19);
        addRoute("Tengboche",  "Dingboche",       500,   16);
        addRoute("Dingboche",  "Lobuche",         400,   11);
        addRoute("Lobuche",    "Gorak Shep",      300,    5);
        addRoute("Gorak Shep", "EBC",             200,    3);

        // Annapurna Circuit
        addRoute("Besisahar",  "Chame",           800,   75);
        addRoute("Chame",      "Pisang",          400,   22);
        addRoute("Pisang",     "Manang",          400,   22);
        addRoute("Manang",     "Thorong La Pass", 600,   18);
        addRoute("Thorong La Pass", "Muktinath",  300,    5);
        addRoute("Muktinath",  "Jomsom",          500,   22);
        addRoute("Jomsom",     "Pokhara",        5000,  160);

        // Langtang
        addRoute("Syabrubesi", "Lama Hotel",      400,   18);
        addRoute("Lama Hotel", "Langtang Village", 400,  14);
        addRoute("Langtang Village", "Kyanjin Gompa", 300, 6);

        // Poon Hill
        addRoute("Nayapul",    "Tikhedhunga",     300,   10);
        addRoute("Tikhedhunga","Ghorepani",        400,   12);
        addRoute("Ghorepani",  "Poon Hill",        200,    1);
        addRoute("Ghorepani",  "Tadapani",         300,    8);
        addRoute("Tadapani",   "Ghandruk",         300,    5);
        addRoute("Ghandruk",   "Nayapul",          400,   20);

        // Mustang
        addRoute("Jomsom",     "Kagbeni",          300,   10);
        addRoute("Kagbeni",    "Chele",            400,   18);
        addRoute("Chele",      "Lo Manthang",      800,   52);
    }

    // Real-world approximate elevations (meters above sea level) for
    // every checkpoint in the graph above.
    void buildAltitudeTable() {
        altitude["Kathmandu"]        = 1400;
        altitude["Pokhara"]          = 822;
        altitude["Chitwan"]          = 150;
        altitude["Lukla"]            = 2860;
        altitude["Syabrubesi"]       = 1503;
        altitude["Besisahar"]        = 760;
        altitude["Lumbini"]          = 150;
        altitude["Jomsom"]           = 2720;
        altitude["Nayapul"]          = 1070;
        altitude["Namche Bazaar"]    = 3440;
        altitude["Tengboche"]        = 3860;
        altitude["Dingboche"]        = 4410;
        altitude["Lobuche"]          = 4940;
        altitude["Gorak Shep"]       = 5164;
        altitude["EBC"]              = 5364;
        altitude["Chame"]            = 2710;
        altitude["Pisang"]           = 3200;
        altitude["Manang"]           = 3540;
        altitude["Thorong La Pass"]  = 5416;
        altitude["Muktinath"]        = 3800;
        altitude["Lama Hotel"]       = 2470;
        altitude["Langtang Village"] = 3430;
        altitude["Kyanjin Gompa"]    = 3870;
        altitude["Tikhedhunga"]      = 1540;
        altitude["Ghorepani"]        = 2860;
        altitude["Poon Hill"]        = 3210;
        altitude["Tadapani"]         = 2630;
        altitude["Ghandruk"]         = 1940;
        altitude["Kagbeni"]          = 2800;
        altitude["Chele"]            = 3050;
        altitude["Lo Manthang"]      = 3840;
    }

    // Add bidirectional route
    void addRoute(string cityA, string cityB, double cost, double distance) {
        graph[cityA].push_back({cityB, cost, distance});
        graph[cityB].push_back({cityA, cost, distance});
    }

    // Returns -1 if the city isn't in the altitude table.
    double getAltitude(const string &city) const {
        auto it = altitude.find(city);
        return (it != altitude.end()) ? it->second : -1.0;
    }

    // Dijkstra's Algorithm - finds the cheapest path between two cities.
    //
    // This is a genuinely *multi-objective* search, not cost-only: the
    // priority queue orders nodes by a combined weight of
    //   combinedWeight = cost + distance * distanceWeightNprPerKm
    // (a standard linear scalarization of the two-objective vector
    // w(e) = (cost, distance)). distanceWeightNprPerKm defaults to 5.0,
    // meaning every km of physical distance is treated as if it added
    // 5 NPR of "effective cost" when comparing candidate paths - so a
    // slightly cheaper but much longer detour can lose out to a
    // marginally pricier, much shorter route. The totalCost and
    // totalDistance reported back on RouteResult are always the real,
    // un-scaled sums along the winning path - the scalarization only
    // affects which path gets chosen, never the numbers shown to the
    // user.
    //
    // maxBudget (optional, NPR): if >= 0, any partial path whose real
    // transport cost would exceed it is pruned during relaxation, so
    // the search never returns a route the trekker explicitly said
    // they can't afford. Pass -1 (default) for no budget limit.
    RouteResult findCheapestRoute(string start, string destination,
                                  double maxBudget = -1,
                                  double distanceWeightNprPerKm = 5.0) {
        // combinedWeight[city] = the scalarized (cost + weighted distance)
        // used purely to drive the priority queue / relaxation order.
        map<string, double> combinedWeight;
        // realCost[city] / realDistance[city] = the actual, un-scaled
        // totals along the best-known path so far - these are what
        // eventually get reported to the caller.
        map<string, double> realCost;
        map<string, double> realDistance;
        map<string, string> previous;

        for (auto& node : graph) {
            combinedWeight[node.first] = numeric_limits<double>::infinity();
            realCost[node.first] = numeric_limits<double>::infinity();
            realDistance[node.first] = numeric_limits<double>::infinity();
        }
        combinedWeight[start] = 0;
        realCost[start] = 0;
        realDistance[start] = 0;

        // Priority queue: (combinedWeight, city) - min heap
        priority_queue<pair<double,string>,
                       vector<pair<double,string>>,
                       greater<pair<double,string>>> pq;
        pq.push({0, start});

        while (!pq.empty()) {
            auto [currentWeight, currentCity] = pq.top();
            pq.pop();

            // Skip if we found a better path already
            if (currentWeight > combinedWeight[currentCity]) continue;

            // Stop early if we reached destination
            if (currentCity == destination) break;

            // Explore neighbors
            for (Edge& edge : graph[currentCity]) {
                double newRealCost = realCost[currentCity] + edge.cost;

                // Budget pruning: never relax into a state that's
                // already unaffordable, regardless of how cheap the
                // rest of the trip might look from there.
                if (maxBudget >= 0 && newRealCost > maxBudget)
                    continue;

                double newWeight = combinedWeight[currentCity]
                                   + edge.cost
                                   + edge.distance * distanceWeightNprPerKm;

                if (newWeight < combinedWeight[edge.destination]) {
                    combinedWeight[edge.destination] = newWeight;
                    realCost[edge.destination] = newRealCost;
                    realDistance[edge.destination] = realDistance[currentCity] + edge.distance;
                    previous[edge.destination] = currentCity;
                    pq.push({newWeight, edge.destination});
                }
            }
        }

        // Build result
        RouteResult result;
        result.budgetLimited = false;

        if (combinedWeight[destination] == numeric_limits<double>::infinity()) {
            result.found = false;
            result.totalCost = 0;
            result.totalDistance = 0;

            // Distinguish "no path exists at all" from "a path exists,
            // but every one of them costs more than maxBudget" - the
            // budget-agnostic search below (maxBudget = -1) tells us
            // whether the destination is reachable in principle.
            if (maxBudget >= 0) {
                RouteResult unlimited = findCheapestRoute(start, destination, -1, distanceWeightNprPerKm);
                if (unlimited.found)
                    result.budgetLimited = true;
            }
            return result;
        }

        result.found = true;
        result.totalCost = realCost[destination];
        result.totalDistance = realDistance[destination];

        // Reconstruct path by backtracking
        vector<string> path;
        string current = destination;
        while (current != start) {
            path.push_back(current);
            current = previous[current];
        }
        path.push_back(start);
        reverse(path.begin(), path.end());
        result.path = path;

        // AMS check: walk the finished path checkpoint-by-checkpoint
        // and flag any single hop that gains more than the safe daily
        // ascent limit. This assumes one hop = roughly one trekking
        // day, which matches how the sample trail segments (e.g.
        // Dingboche -> Lobuche, Namche Bazaar -> Tengboche) are laid
        // out in buildNepalGraph().
        for (size_t i = 0; i + 1 < path.size(); i++) {
            double fromAlt = getAltitude(path[i]);
            double toAlt = getAltitude(path[i + 1]);
            if (fromAlt < 0 || toAlt < 0) continue; // no altitude data for this checkpoint

            double gain = toAlt - fromAlt;
            if (gain > AMS_SAFE_ASCENT_METERS) {
                result.amsWarnings.push_back({path[i], path[i + 1], gain});
            }
        }

        return result;
    }

    // Get all cities available in the graph
    vector<string> getAllCities() {
        vector<string> cities;
        for (auto& node : graph)
            cities.push_back(node.first);
        return cities;
    }
};

#endif // ROUTEOPTIMIZER_H