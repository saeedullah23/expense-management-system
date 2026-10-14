// main.cpp - console menu (the main application) + optional "--web" mode.
//   ./ems            -> console version
//   ./ems --web      -> starts the C++ web server on http://localhost:8080
//   ./ems --web 9000 -> same, custom port
#include "ExpenseManager.h"
#include "Report.h"
#include "Utils.h"
#include "WebServer.h"
#include <cstring>
#include <iomanip>
#include <iostream>
using namespace std;

// ---------- small input helpers built on Utils ----------
static void askMonthYear(int& m, int& y) {
    string t = Utils::todayISO();
    m = Utils::readInt("Month 1-12 (Enter = " + Utils::monthName(Utils::monthOf(t)) + "): ", 1, 12, true, Utils::monthOf(t));
    y = Utils::readInt("Year (Enter = " + to_string(Utils::yearOf(t)) + "): ", 2000, 2100, true, Utils::yearOf(t));
}

static string askDate(const string& prompt, const string& defIso) {
    while (true) {
        string s = Utils::readLine(prompt);
        string iso;
        if (s.empty() && !defIso.empty()) return defIso;
        if (Utils::parseDate(s, iso)) return iso;
        cout << "Invalid input. Please try again.\n";
    }
}

static string pickFromList(const string& title, const vector<string>& list) {
    cout << title << "\n";
    for (size_t i = 0; i < list.size(); i++) cout << "  " << i + 1 << ". " << list[i] << "\n";
    return list[Utils::readInt("Choose: ", 1, (int)list.size()) - 1];
}

static void showAlert(const ExpenseManager& m, int month, int year) {
    MonthSummary s = m.summary(month, year);
    string a = ExpenseManager::alertMessage(s);
    if (a.empty()) return;
    cout << "\n*** " << a << " ***\n";
    if (s.percent >= 100) cout << '\a';          // terminal beep for "reached/exceeded"
}

// ---------- menu actions ----------
static void addExpense(ExpenseManager& m) {
    cout << "\n--- Add Expense ---\n";
    string date = askDate("Date DD/MM/YYYY (Enter = today): ", Utils::todayISO());
    double amount = Utils::readAmount("Amount: ");
    string cat = pickFromList("Category:", Expense::categories());
    string desc = Utils::readLine("Description: ");
    string pay = pickFromList("Payment method:", Expense::paymentMethods());
    int id = m.addExpense(date, amount, cat, desc, pay);
    cout << "Expense added successfully (ID " << id << ").\n";
    showAlert(m, Utils::monthOf(date), Utils::yearOf(date));
}

static void viewExpenses(ExpenseManager& m) {
    cout << "\n--- View Expenses ---\n";
    int month = Utils::readInt("Month 1-12 (0 = all months): ", 0, 12);
    int year = 0;
    if (month != 0) year = Utils::readInt("Year: ", 2000, 2100);
    cout << "Sort by: 1) Date  2) Amount  3) Category\n";
    int f = Utils::readInt("Choose (Enter = Date): ", 1, 3, true, 1);
    cout << "Order: 1) Ascending  2) Descending\n";
    int o = Utils::readInt("Choose (Enter = Descending): ", 1, 2, true, 2);
    auto list = m.monthExpenses(month, year);
    ExpenseManager::sortExpenses(list, f == 2 ? SortField::Amount : f == 3 ? SortField::Category : SortField::Date, o == 1);
    cout << "\n";
    Report::printExpenseTable(list);
}

static void editExpense(ExpenseManager& m) {
    cout << "\n--- Edit Expense ---\n";
    auto all = m.userExpenses();
    ExpenseManager::sortExpenses(all, SortField::Date, false);
    Report::printExpenseTable(all);
    if (all.empty()) return;
    Expense e;
    if (!m.findExpense(Utils::readInt("Expense ID to edit: ", 1, 99999999), e)) { cout << "Expense not found.\n"; return; }
    cout << "Press Enter to keep the current value.\n";
    string date = askDate("Date [" + Utils::isoToDisplay(e.getDate()) + "]: ", e.getDate());
    string amt = Utils::readLine("Amount [" + to_string((long long)e.getAmount()) + "]: ");
    double amount = e.getAmount();
    if (!amt.empty() && !Utils::parseAmount(amt, amount)) { cout << "Invalid input. Please try again.\n"; return; }
    string cat = Utils::readLine("Category [" + e.getCategory() + "] (type name): ");
    string desc = Utils::readLine("Description [" + e.getDescription() + "]: ");
    string pay = Utils::readLine("Payment [" + e.getPaymentMethod() + "] (type name): ");
    m.editExpense(e.getId(), date, amount, cat.empty() ? e.getCategory() : cat,
                  desc.empty() ? e.getDescription() : desc, pay.empty() ? e.getPaymentMethod() : pay);
    cout << "Expense updated successfully.\n";
    showAlert(m, Utils::monthOf(date), Utils::yearOf(date));
}

static void deleteExpense(ExpenseManager& m) {
    cout << "\n--- Delete Expense ---\n";
    auto all = m.userExpenses();
    ExpenseManager::sortExpenses(all, SortField::Date, false);
    Report::printExpenseTable(all);
    if (all.empty()) return;
    Expense e;
    if (!m.findExpense(Utils::readInt("Expense ID to delete: ", 1, 99999999), e)) { cout << "Expense not found.\n"; return; }
    string a = Utils::toLower(Utils::readLine("Are you sure you want to delete this expense? (yes/no): "));
    if (a == "y" || a == "yes") { m.deleteExpense(e.getId()); cout << "Expense deleted successfully.\n"; }
    else cout << "Deletion cancelled.\n";
}

static void setBudget(ExpenseManager& m) {
    cout << "\n--- Set Monthly Budget ---\n";
    int month, year;
    askMonthYear(month, year);
    double a = Utils::readAmount("Enter Monthly Budget: Rs. ");
    m.setBudget(month, year, a);
    cout << "Budget for " << Utils::monthName(month) << " " << year << " saved: " << Utils::formatMoney(a) << "\n";
    showAlert(m, month, year);
}

static void viewBudget(ExpenseManager& m) {
    cout << "\n--- View Budget ---\n";
    int month, year;
    askMonthYear(month, year);
    MonthSummary s = m.summary(month, year);
    cout << "\n" << Utils::monthName(month) << " " << year << "\n";
    if (!s.hasBudget) { cout << "No budget set for this month. Spent so far: " << Utils::formatMoney(s.spent) << "\n"; return; }
    cout << "Monthly Budget : " << Utils::formatMoney(s.budget) << "\nTotal Spent    : " << Utils::formatMoney(s.spent)
         << "\nRemaining      : " << Utils::formatMoney(s.remaining) << "\nBudget Used    : ";
    Report::budgetBar(s.percent);
    cout << "Status         : " << s.status << "\n";
    showAlert(m, month, year);
}

static void searchExpense(ExpenseManager& m) {
    cout << "\n--- Search Expense (date, category, description) ---\n";
    string k = Utils::readLine("Search: ");
    Report::printExpenseTable(ExpenseManager::search(m.userExpenses(), k));
}

static void monthlyHistory(ExpenseManager& m) {
    cout << "\n=========== MONTHLY HISTORY ===========\n";
    auto h = m.history();
    if (h.empty()) { cout << "No data yet.\n"; return; }
    cout << left << setw(4) << "#" << setw(16) << "Month" << right << setw(14) << "Budget" << setw(14) << "Spent"
         << setw(14) << "Remaining" << "  " << left << "Status\n" << string(74, '-') << "\n";
    for (size_t i = 0; i < h.size(); i++) {
        const auto& s = h[i];
        cout << left << setw(4) << i + 1 << setw(16) << (Utils::monthName(s.month).substr(0, 3) + " " + to_string(s.year)) << right
             << setw(14) << (s.hasBudget ? Utils::formatMoney(s.budget) : "-") << setw(14) << Utils::formatMoney(s.spent)
             << setw(14) << (s.hasBudget ? Utils::formatMoney(s.remaining) : "-") << "  " << left << s.status << "\n";
    }
    int pick = Utils::readInt("\nEnter # to see that month's full report (Enter = back): ", 1, (int)h.size(), true, 0);
    if (pick == 0) return;
    const auto& s = h[pick - 1];
    Report::monthlyReport(m, s.month, s.year);
    Report::categoryReport(m, s.month, s.year);
    auto list = m.monthExpenses(s.month, s.year);
    ExpenseManager::sortExpenses(list, SortField::Date, true);
    cout << "\n";
    Report::printExpenseTable(list);
    string ex = Utils::toLower(Utils::readLine("\nExport this month to a CSV file? (yes/no): "));
    if (ex == "y" || ex == "yes") cout << "Saved to " << m.exportMonth(s.month, s.year) << "\n";
}

// ---------- menus ----------
static void userMenu(ExpenseManager& m) {
    while (m.isLoggedIn()) {
        cout << "\n=====================================\n       EXPENSE MANAGEMENT SYSTEM\n=====================================\n"
             << "Logged in as: " << m.currentName() << "\n\n"
             << " 1. Add Expense\n 2. View Expenses\n 3. Edit Expense\n 4. Delete Expense\n 5. Set Monthly Budget\n"
             << " 6. View Budget\n 7. Daily Report\n 8. Monthly Report\n 9. Category Report\n10. Search Expense\n"
             << "11. Monthly History\n12. Logout\n13. Exit\n\n";
        int c = Utils::readInt("Enter choice: ", 1, 13);
        try {                                            // exception handling: any error is reported, program keeps running
            int mo, yr;
            switch (c) {
                case 1: addExpense(m); break;
                case 2: viewExpenses(m); break;
                case 3: editExpense(m); break;
                case 4: deleteExpense(m); break;
                case 5: setBudget(m); break;
                case 6: viewBudget(m); break;
                case 7: Report::dailyReport(m, askDate("Date DD/MM/YYYY (Enter = today): ", Utils::todayISO())); break;
                case 8: askMonthYear(mo, yr); Report::monthlyReport(m, mo, yr); break;
                case 9: askMonthYear(mo, yr); Report::categoryReport(m, mo, yr); break;
                case 10: searchExpense(m); break;
                case 11: monthlyHistory(m); break;
                case 12: m.logout(); cout << "Logged out.\n"; return;
                case 13: cout << "Goodbye!\n"; exit(0);
            }
        } catch (const exception& e) {
            cout << "Invalid input. Please try again. (" << e.what() << ")\n";
        }
        if (m.isLoggedIn()) Utils::pauseScreen();
    }
}

static void authMenu(ExpenseManager& m) {
    while (true) {
        cout << "\n=====================================\n       EXPENSE MANAGEMENT SYSTEM\n=====================================\n"
             << "1. Login\n2. Register\n3. Exit\n\n";
        int c = Utils::readInt("Enter choice: ", 1, 3);
        try {
            if (c == 1) {
                string u = Utils::readLine("Username: "), p = Utils::readLine("Password: ");
                if (m.login(u, p)) { cout << "Login successful!\nWelcome " << m.currentName() << "!\n"; userMenu(m); }
                else cout << "Invalid username or password.\n";
            } else if (c == 2) {
                string n = Utils::readLine("Full name: "), u = Utils::readLine("Username: "), p = Utils::readLine("Password: ");
                m.registerUser(n, u, p);
                cout << "Registration successful!\n";
            } else { cout << "Goodbye!\n"; return; }
        } catch (const exception& e) {
            cout << "Error: " << e.what() << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        ExpenseManager manager;                           // loads data/*.txt
        if (argc >= 2 && strcmp(argv[1], "--web") == 0) {
            int port = (argc >= 3) ? atoi(argv[2]) : 8080;
            WebServer server(manager, port > 0 ? port : 8080);
            server.run();
        } else {
            authMenu(manager);
        }
    } catch (const exception& e) {
        cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
