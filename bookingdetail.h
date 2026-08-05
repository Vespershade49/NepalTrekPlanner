#ifndef BOOKINGDETAIL_H
#define BOOKINGDETAIL_H
#include <string>
using namespace std;

// Matches the columns returned by View_Booking_Details
class BookingDetail {
public:
    int bookingId;
    string userName;
    string trekName;
    string region;
    string difficultyLevel;
    string startDate;
    int numPeople;
    double totalCost;
    string status;
    string assignedGuides;

    BookingDetail() {}
    BookingDetail(int id, string user, string trek, string reg, string diff,
                  string date, int people, double cost, string stat, string guides)
        : bookingId(id), userName(user), trekName(trek), region(reg),
        difficultyLevel(diff), startDate(date), numPeople(people),
        totalCost(cost), status(stat), assignedGuides(guides) {}
};

#endif // BOOKINGDETAIL_H
