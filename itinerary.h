#ifndef ITINERARY_H
#define ITINERARY_H

#include <string>
#include <vector>
using namespace std;

// Matches the ER diagram in the proposal:
// "Each Transaction links an Itinerary to either a TouristSpot or a TrekRoute."
class Transaction {
public:
    int transactionId;
    int itineraryId;
    int refId;      // the trek_id or spot_id this transaction points to
    string type;    // "spot" or "trek"

    Transaction() {}
    Transaction(int tid, int iid, int rid, string t)
        : transactionId(tid), itineraryId(iid), refId(rid), type(t) {}
};

// Matches the ER diagram's Itinerary entity:
// itinerary_id, user_id, total_cost, created_date
class Itinerary {
public:
    int itineraryId;
    int userId;
    double totalCost;
    string createdDate;
    vector<Transaction> transactions; // the linked TrekRoute + TouristSpots

    Itinerary() {}
    Itinerary(int id, int uid, double cost, string date)
        : itineraryId(id), userId(uid), totalCost(cost), createdDate(date) {}

    // Adds one linked trek or spot as a Transaction row
    void addTransaction(int refId, string type) {
        int nextTransactionId = (int)transactions.size() + 1;
        transactions.push_back(Transaction(nextTransactionId, itineraryId, refId, type));
    }
};

#endif // ITINERARY_H