#ifndef ROUTEOPTIMIZER_H
#define ROUTEOPTIMIZER_H

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <limits>
using namespace std;

// Represents a road/path between two cities
struct Edge {
    string destination;
    double cost;      // transport cost in NPR
    double distance;  // in km
};

// Result of Dijkstra
struct RouteResult {
    vector<string> path;       // cities in order
    double totalCost;          // total transport cost in NPR
    double totalDistance;      // total km
    bool found;                // false if no path exists
};

class RouteOptimizer {
private:
    // Adjacency list: city -> list of connected cities with cost
    map<string, vector<Edge>> graph;

public:
    RouteOptimizer() {
        buildNepalGraph();
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

    // Add bidirectional route
    void addRoute(string cityA, string cityB, double cost, double distance) {
        graph[cityA].push_back({cityB, cost, distance});
        graph[cityB].push_back({cityA, cost, distance});
    }

    // Dijkstra's Algorithm - finds cheapest path between two cities
    RouteResult findCheapestRoute(string start, string destination) {
        // Distance/cost map - initialized to infinity
        map<string, double> minCost;
        map<string, string> previous;  // to reconstruct path

        for (auto& node : graph)
            minCost[node.first] = numeric_limits<double>::infinity();
        minCost[start] = 0;

        // Priority queue: (cost, city) - min heap
        priority_queue<pair<double,string>,
                       vector<pair<double,string>>,
                       greater<pair<double,string>>> pq;
        pq.push({0, start});

        while (!pq.empty()) {
            auto [currentCost, currentCity] = pq.top();
            pq.pop();

            // Skip if we found a better path already
            if (currentCost > minCost[currentCity]) continue;

            // Stop early if we reached destination
            if (currentCity == destination) break;

            // Explore neighbors
            for (Edge& edge : graph[currentCity]) {
                double newCost = minCost[currentCity] + edge.cost;
                if (newCost < minCost[edge.destination]) {
                    minCost[edge.destination] = newCost;
                    previous[edge.destination] = currentCity;
                    pq.push({newCost, edge.destination});
                }
            }
        }

        // Build result
        RouteResult result;
        result.totalCost = minCost[destination];

        if (minCost[destination] == numeric_limits<double>::infinity()) {
            result.found = false;
            return result;
        }

        result.found = true;

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

        // Calculate total distance
        result.totalDistance = 0;
        for (int i = 0; i < (int)path.size() - 1; i++) {
            for (Edge& edge : graph[path[i]]) {
                if (edge.destination == path[i+1]) {
                    result.totalDistance += edge.distance;
                    break;
                }
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
