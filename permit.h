#ifndef PERMIT_H
#define PERMIT_H
#include <string>
using namespace std;

class Permit {
public:
    int permitId;
    int trekId;
    string permitName;
    double cost;
    string issuingAuthority;

    Permit() {}
    Permit(int pid, int tid, string name, double c, string auth)
        : permitId(pid), trekId(tid), permitName(name), cost(c), issuingAuthority(auth) {}
};

#endif // PERMIT_H