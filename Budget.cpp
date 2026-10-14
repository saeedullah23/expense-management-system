// Budget.cpp
#include "Budget.h"
#include "Utils.h"
#include <cstdio>
#include <iostream>
#include <stdexcept>
using namespace std;

Budget::Budget(const string& user, int m, int y, double a) : username(user), month(m), year(y), amount(0) {
    if (m < 1 || m > 12 || y < 2000 || y > 2100) throw invalid_argument("Invalid month or year.");
    setAmount(a);
}

void Budget::setAmount(double a) {
    if (!(a > 0) || a > 1e9) throw invalid_argument("Budget must be a positive number.");
    amount = a;
}

string Budget::toLine() const {
    char amt[32];
    snprintf(amt, sizeof amt, "%.2f", amount);
    return username + "|" + to_string(month) + "|" + to_string(year) + "|" + amt;
}

Budget Budget::fromLine(const string& line) {
    auto f = Utils::split(line, '|');
    if (f.size() != 4) throw runtime_error("bad budget record");
    return Budget(f[0], stoi(f[1]), stoi(f[2]), stod(f[3]));
}

void Budget::display() const {
    cout << Utils::monthName(month) << " " << year << "  " << Utils::formatMoney(amount) << "\n";
}
