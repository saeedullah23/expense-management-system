// Expense.h - one spending record. All fields are private (encapsulation);
// setters validate the data and throw std::invalid_argument when it is wrong.
#ifndef EXPENSE_H
#define EXPENSE_H
#include "Record.h"
#include <string>
#include <vector>

class Expense : public Record {
private:
    int id;
    std::string username;       // owner of the record -> keeps users' data separate
    std::string date;           // ISO YYYY-MM-DD
    double amount;
    std::string category;
    std::string description;
    std::string paymentMethod;
public:
    Expense();
    Expense(int id, const std::string& user, const std::string& date, double amount,
            const std::string& category, const std::string& description, const std::string& payment);

    // getters
    int getId() const { return id; }
    const std::string& getUsername() const { return username; }
    const std::string& getDate() const { return date; }
    double getAmount() const { return amount; }
    const std::string& getCategory() const { return category; }
    const std::string& getDescription() const { return description; }
    const std::string& getPaymentMethod() const { return paymentMethod; }

    // validating setters
    void setDate(const std::string& d);
    void setAmount(double a);
    void setCategory(const std::string& c);
    void setDescription(const std::string& d);
    void setPaymentMethod(const std::string& p);

    static const std::vector<std::string>& categories();
    static const std::vector<std::string>& paymentMethods();
    static Expense fromLine(const std::string& line);

    std::string toLine() const override;
    void display() const override;
};
#endif
