#ifndef REVIEW_H
#define REVIEW_H
#include <string>
using namespace std;

class Review {
public:
    int reviewId;
    string userName;
    string trekName;
    int rating;
    string comment;
    string reviewedAt;

    Review() {}
    Review(int id, string user, string trek, int r, string c, string date)
        : reviewId(id), userName(user), trekName(trek), rating(r),
        comment(c), reviewedAt(date) {}
};

#endif // REVIEW_H
