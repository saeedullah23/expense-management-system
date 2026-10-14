// Expense.cpp
#include "Expense.h"
#include "Utils.h"
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace std;

const vector<string>& Expense::categories() {
    static const vector<string> c = {"Food", "Transport", "Shopping", "Bills", "Education",
                                     "Health", "Entertainment", "Rent", "Groceries", "Other"};
    return c;
}
const vector<string>& Expense::paymentMethods() {
    static const vector<string> p = {"Cash", "Card", "Bank Transfer", "Mobile Wallet", "Other"};
    return p;
}

// finds the official spelling of a value in a list (case-insensitive)
static bool canonical(const vector<string>& list, const string& v, string& out) {
    string t = Utils::toLower(Utils::trim(v));
    for (const auto& x : list) if (Utils::toLower(x) == t) { out = x; return true; }
    return false;
}

Expense::Expense() : id(0), amount(0) {}

Expense::Expense(int i, const string& user, const string& d, double a,
                 const string& c, const string& desc, const string& p) : id(i), username(user), amount(0) {
    setDate(d); setAmount(a); setCategory(c); setDescription(desc); setPaymentMethod(p);
}

void Expense::setDate(const string& d) {
    string iso;
    if (!Utils::parseDate(d, iso)) throw invalid_argument("Invalid date (use DD/MM/YYYY).");
    date = iso;
}
void Expense::setAmount(double a) {
    if (!(a > 0) || a > 1e9) throw invalid_argument("Amount must be a positive number.");
    amount = a;
}
void Expense::setCategory(const string& c) {
    string out;
    if (!canonical(categories(), c, out)) throw invalid_argument("Unknown category.");
    category = out;
}
void Expense::setDescription(const string& d) {
    string t = Utils::sanitize(d);
    description = t.empty() ? "-" : t.substr(0, 80);
}
void Expense::setPaymentMethod(const string& p) {
    string out;
    if (!canonical(paymentMethods(), p, out)) throw invalid_argument("Unknown payment method.");
    paymentMethod = out;
}

string Expense::toLine() const {
    char amt[32];
    snprintf(amt, sizeof amt, "%.2f", amount);
    return to_string(id) + "|" + username + "|" + date + "|" + amt + "|" + category + "|" + description + "|" + paymentMethod;
}

Expense Expense::fromLine(const string& line) {
    auto f = Utils::split(line, '|');
    if (f.size() != 7) throw runtime_error("bad expense record");
    return Expense(stoi(f[0]), f[1], f[2], stod(f[3]), f[4], f[5], f[6]);
}

void Expense::display() const {
    string d = description.size() > 22 ? description.substr(0, 21) + "~" : description;
    cout << left << setw(5) << id << setw(12) << Utils::isoToDisplay(date) << setw(15) << category
         << setw(24) << d << setw(16) << paymentMethod << right << setw(12) << Utils::formatMoney(amount) << "\n";
}
