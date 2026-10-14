// ExpenseManager.cpp
#include "ExpenseManager.h"
#include "FileManager.h"
#include "Utils.h"
#include <algorithm>
#include <set>
#include <stdexcept>
using namespace std;

ExpenseManager::ExpenseManager() : nextId(1) {
    FileManager::ensureFiles();
    users = FileManager::loadUsers();
    budgets = FileManager::loadBudgets();
    expenses = FileManager::loadExpenses();
    for (const auto& e : expenses) nextId = max(nextId, e.getId() + 1);
}

void ExpenseManager::requireLogin() const {
    if (!isLoggedIn()) throw runtime_error("Please login first.");
}

// ---------------- accounts ----------------
void ExpenseManager::registerUser(const string& name, const string& username, const string& password) {
    User u = User::create(name, username, password);             // validates (throws on error)
    for (const auto& x : users)
        if (Utils::toLower(x.getUsername()) == Utils::toLower(u.getUsername()))
            throw runtime_error("Username already exists.");
    users.push_back(u);
    FileManager::saveUsers(users);
}

bool ExpenseManager::login(const string& username, const string& password) {
    string key = Utils::toLower(Utils::trim(username));
    for (const auto& u : users) {
        if (Utils::toLower(u.getUsername()) == key && u.checkPassword(password)) {
            currentUser = u.getUsername();
            return true;
        }
    }
    return false;
}

void ExpenseManager::setCurrentUser(const string& username) {
    for (const auto& u : users) if (u.getUsername() == username) { currentUser = username; return; }
    throw runtime_error("Unknown user.");
}

string ExpenseManager::currentName() const {
    for (const auto& u : users) if (u.getUsername() == currentUser) return u.getName();
    return "";
}

// ---------------- budget ----------------
void ExpenseManager::setBudget(int month, int year, double amount) {
    requireLogin();
    Budget b(currentUser, month, year, amount);                   // validates
    for (auto& x : budgets)
        if (x.getUsername() == currentUser && x.getMonth() == month && x.getYear() == year) {
            x.setAmount(amount);
            FileManager::saveBudgets(budgets);
            return;
        }
    budgets.push_back(b);
    FileManager::saveBudgets(budgets);
}

bool ExpenseManager::getBudget(int month, int year, double& out) const {
    for (const auto& x : budgets)
        if (x.getUsername() == currentUser && x.getMonth() == month && x.getYear() == year) {
            out = x.getBudget();
            return true;
        }
    return false;
}

// ---------------- expenses ----------------
int ExpenseManager::addExpense(const string& date, double amount, const string& category,
                               const string& description, const string& payment) {
    requireLogin();
    Expense e(nextId, currentUser, date, amount, category, description, payment);   // validates
    expenses.push_back(e);
    FileManager::appendExpense(e);
    return nextId++;
}

int ExpenseManager::findExpenseIndex(int id) const {
    for (size_t i = 0; i < expenses.size(); i++)
        if (expenses[i].getId() == id && expenses[i].getUsername() == currentUser) return (int)i;
    return -1;
}

bool ExpenseManager::findExpense(int id, Expense& out) const {
    int i = findExpenseIndex(id);
    if (i < 0) return false;
    out = expenses[i];
    return true;
}

void ExpenseManager::editExpense(int id, const string& date, double amount, const string& category,
                                 const string& description, const string& payment) {
    requireLogin();
    int i = findExpenseIndex(id);
    if (i < 0) throw runtime_error("Expense not found.");
    Expense copy = expenses[i];                                   // edit a copy first: if validation fails nothing changes
    copy.setDate(date); copy.setAmount(amount); copy.setCategory(category);
    copy.setDescription(description); copy.setPaymentMethod(payment);
    expenses[i] = copy;
    FileManager::saveExpenses(expenses);
}

void ExpenseManager::deleteExpense(int id) {
    requireLogin();
    int i = findExpenseIndex(id);
    if (i < 0) throw runtime_error("Expense not found.");
    expenses.erase(expenses.begin() + i);
    FileManager::saveExpenses(expenses);
}

vector<Expense> ExpenseManager::userExpenses() const {
    requireLogin();
    vector<Expense> r;
    for (const auto& e : expenses) if (e.getUsername() == currentUser) r.push_back(e);
    return r;
}

vector<Expense> ExpenseManager::monthExpenses(int month, int year) const {
    vector<Expense> r;
    for (const auto& e : userExpenses())
        if (month == 0 || (Utils::monthOf(e.getDate()) == month && Utils::yearOf(e.getDate()) == year)) r.push_back(e);
    return r;
}

vector<Expense> ExpenseManager::dayExpenses(const string& iso) const {
    vector<Expense> r;
    for (const auto& e : userExpenses()) if (e.getDate() == iso) r.push_back(e);
    return r;
}

// ---------------- search & sort ----------------
vector<Expense> ExpenseManager::search(const vector<Expense>& src, const string& keyword) {
    string k = Utils::toLower(Utils::trim(keyword));
    if (k.empty()) return src;
    vector<Expense> r;
    for (const auto& e : src) {
        string hay = Utils::toLower(e.getDate() + " " + Utils::isoToDisplay(e.getDate()) + " " + e.getCategory() +
                                    " " + e.getDescription() + " " + e.getPaymentMethod());
        if (hay.find(k) != string::npos) r.push_back(e);
    }
    return r;
}

void ExpenseManager::sortExpenses(vector<Expense>& list, SortField field, bool asc) {
    stable_sort(list.begin(), list.end(), [field, asc](const Expense& a, const Expense& b) {
        int cmp = 0;
        if (field == SortField::Amount) cmp = (a.getAmount() < b.getAmount()) ? -1 : (a.getAmount() > b.getAmount());
        else if (field == SortField::Category) cmp = a.getCategory().compare(b.getCategory());
        else cmp = a.getDate().compare(b.getDate());
        if (cmp == 0) cmp = (a.getId() < b.getId()) ? -1 : (a.getId() > b.getId());   // tie-breaker
        return asc ? cmp < 0 : cmp > 0;
    });
}

// ---------------- calculations ----------------
double ExpenseManager::total(const vector<Expense>& list) {
    double t = 0;
    for (const auto& e : list) t += e.getAmount();
    return t;
}

map<string, double> ExpenseManager::categoryTotals(int month, int year) const {
    map<string, double> m;
    for (const auto& e : monthExpenses(month, year)) m[e.getCategory()] += e.getAmount();
    return m;
}

vector<double> ExpenseManager::dailyTotals(int month, int year) const {
    vector<double> d(Utils::daysInMonth(month, year), 0.0);
    for (const auto& e : monthExpenses(month, year)) {
        int day = Utils::dayOf(e.getDate());
        if (day >= 1 && day <= (int)d.size()) d[day - 1] += e.getAmount();
    }
    return d;
}

string ExpenseManager::statusFor(bool hasBudget, double p) {
    if (!hasBudget) return "No Budget";
    if (p > 100.0) return "Exceeded";
    if (p >= 100.0) return "Limit Reached";
    if (p >= 90.0) return "Near Limit";
    if (p >= 80.0) return "Warning";
    return "Safe";
}

string ExpenseManager::alertMessage(const MonthSummary& s) {
    if (!s.hasBudget) return "";
    if (s.percent > 100.0) return "ALERT: Your monthly budget has been exceeded!";
    if (s.percent >= 100.0) return "ALERT: Your monthly budget has been reached!";
    if (s.percent >= 90.0) return "WARNING: You have used 90% of your monthly budget.";
    if (s.percent >= 80.0) return "WARNING: You have used 80% of your monthly budget.";
    return "";
}

MonthSummary ExpenseManager::summary(int month, int year) const {
    MonthSummary s;
    s.month = month; s.year = year;
    s.spent = total(monthExpenses(month, year));
    s.hasBudget = getBudget(month, year, s.budget);
    if (s.hasBudget) {
        Budget b(currentUser, month, year, s.budget);
        s.remaining = b.calculateRemaining(s.spent);        // remainingBudget = monthlyBudget - totalExpenses
        s.percent = b.calculatePercentUsed(s.spent);        // (totalExpenses / monthlyBudget) * 100
    }
    s.status = statusFor(s.hasBudget, s.percent);
    return s;
}

vector<MonthSummary> ExpenseManager::history() const {
    requireLogin();
    set<pair<int, int>> periods;                            // (year, month), automatically sorted
    for (const auto& b : budgets) if (b.getUsername() == currentUser) periods.insert({b.getYear(), b.getMonth()});
    for (const auto& e : userExpenses()) periods.insert({Utils::yearOf(e.getDate()), Utils::monthOf(e.getDate())});
    vector<MonthSummary> r;
    for (const auto& p : periods) r.push_back(summary(p.second, p.first));
    return r;
}

string ExpenseManager::exportMonth(int month, int year) const {
    requireLogin();
    vector<Expense> list = monthExpenses(month, year);
    sortExpenses(list, SortField::Date, true);
    char ym[16];
    snprintf(ym, sizeof ym, "%04d-%02d", year, month);
    string file = FileManager::dataDir + "/export_" + currentUser + "_" + ym + ".csv";
    if (!FileManager::exportCsv(file, list)) throw runtime_error("Could not write export file.");
    return file;
}
