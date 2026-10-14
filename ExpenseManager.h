// ExpenseManager.h - the "brain" of the application. It owns the data (vectors) and all business rules:
// authentication, budgets, expense CRUD, searching, sorting, totals and alerts.
// Both the console menu (main.cpp) and the web API (WebServer.cpp) call this one class.
#ifndef EXPENSEMANAGER_H
#define EXPENSEMANAGER_H
#include "Budget.h"
#include "Expense.h"
#include "User.h"
#include <map>
#include <string>
#include <vector>

struct MonthSummary {                 // result of the monthly calculations
    int month = 0, year = 0;
    bool hasBudget = false;
    double budget = 0, spent = 0, remaining = 0, percent = 0;
    std::string status;               // No Budget / Safe / Warning / Near Limit / Limit Reached / Exceeded
};

enum class SortField { Date, Amount, Category };

class ExpenseManager {
private:
    std::vector<User> users;
    std::vector<Budget> budgets;
    std::vector<Expense> expenses;
    std::string currentUser;          // empty = nobody logged in
    int nextId;

    void requireLogin() const;
    int findExpenseIndex(int id) const;   // only finds expenses owned by the logged-in user
public:
    ExpenseManager();                     // loads all files

    // ---- accounts ----
    void registerUser(const std::string& name, const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
    void logout() { currentUser.clear(); }
    bool isLoggedIn() const { return !currentUser.empty(); }
    void setCurrentUser(const std::string& username);       // used by the web layer after token check
    const std::string& currentUsername() const { return currentUser; }
    std::string currentName() const;

    // ---- budget ----
    void setBudget(int month, int year, double amount);
    bool getBudget(int month, int year, double& out) const;

    // ---- expenses ----
    int addExpense(const std::string& date, double amount, const std::string& category,
                   const std::string& description, const std::string& payment);
    void editExpense(int id, const std::string& date, double amount, const std::string& category,
                     const std::string& description, const std::string& payment);
    void deleteExpense(int id);
    bool findExpense(int id, Expense& out) const;
    std::vector<Expense> userExpenses() const;
    std::vector<Expense> monthExpenses(int month, int year) const;   // month 0 = all months
    std::vector<Expense> dayExpenses(const std::string& isoDate) const;

    // ---- searching / sorting ----
    static std::vector<Expense> search(const std::vector<Expense>& src, const std::string& keyword);
    static void sortExpenses(std::vector<Expense>& list, SortField field, bool ascending);

    // ---- calculations ----
    static double total(const std::vector<Expense>& list);
    std::map<std::string, double> categoryTotals(int month, int year) const;
    std::vector<double> dailyTotals(int month, int year) const;
    MonthSummary summary(int month, int year) const;
    std::vector<MonthSummary> history() const;
    static std::string statusFor(bool hasBudget, double percent);
    static std::string alertMessage(const MonthSummary& s);   // "" when below 80%

    // ---- export ----
    std::string exportMonth(int month, int year) const;       // writes data/export_<user>_<yyyy-mm>.csv, returns path
};
#endif
