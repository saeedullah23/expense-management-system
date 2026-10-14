// Budget.h - the monthly budget of one user for one month/year.
#ifndef BUDGET_H
#define BUDGET_H
#include "Record.h"
#include <string>

class Budget : public Record {
private:
    std::string username;
    int month;
    int year;
    double amount;
public:
    Budget() : month(0), year(0), amount(0) {}
    Budget(const std::string& user, int month, int year, double amount);   // validates

    const std::string& getUsername() const { return username; }
    int getMonth() const { return month; }
    int getYear() const { return year; }
    double getBudget() const { return amount; }
    void setAmount(double a);

    double calculateRemaining(double totalSpent) const { return amount - totalSpent; }
    double calculatePercentUsed(double totalSpent) const { return amount > 0 ? totalSpent / amount * 100.0 : 0.0; }

    static Budget fromLine(const std::string& line);
    std::string toLine() const override;
    void display() const override;
};
#endif
