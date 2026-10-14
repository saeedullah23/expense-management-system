// Report.cpp
#include "Report.h"
#include "Utils.h"
#include <iomanip>
#include <iostream>
using namespace std;

void Report::printExpenseTable(const vector<Expense>& list) {
    if (list.empty()) { cout << "  (no expenses found)\n"; return; }
    cout << left << setw(5) << "ID" << setw(12) << "Date" << setw(15) << "Category" << setw(24) << "Description"
         << setw(16) << "Payment" << right << setw(12) << "Amount" << "\n" << string(84, '-') << "\n";
    for (const auto& e : list) e.display();
    cout << string(84, '-') << "\n" << right << setw(84) << ("Total: " + Utils::formatMoney(ExpenseManager::total(list))) << "\n";
}

void Report::budgetBar(double percent) {
    int filled = (int)(min(percent, 100.0) / 5.0);        // 20 slots
    cout << "[" << string(filled, '#') << string(20 - filled, '.') << "] " << fixed << setprecision(1) << percent << "%\n";
}

void Report::dailyReport(const ExpenseManager& m, const string& iso) {
    vector<Expense> list = m.dayExpenses(iso);
    cout << "\n================================\nDAILY EXPENSE REPORT\n================================\n";
    cout << "Date: " << Utils::dayOf(iso) << " " << Utils::monthName(Utils::monthOf(iso)) << " " << Utils::yearOf(iso) << "\n\n";
    cout << "Total Expenses Today:\n" << Utils::formatMoney(ExpenseManager::total(list)) << "\n\n";
    cout << "Number of Transactions:\n" << list.size() << "\n\n";
    printExpenseTable(list);
}

void Report::monthlyReport(const ExpenseManager& m, int month, int year) {
    MonthSummary s = m.summary(month, year);
    cout << "\n================================\nMONTHLY EXPENSE REPORT\n================================\n";
    cout << "Month: " << Utils::monthName(month) << " " << year << "\n\n";
    cout << "Budget:\n" << (s.hasBudget ? Utils::formatMoney(s.budget) : "Not set") << "\n\n";
    cout << "Total Spent:\n" << Utils::formatMoney(s.spent) << "\n\n";
    if (s.hasBudget) {
        cout << "Remaining:\n" << Utils::formatMoney(s.remaining) << "\n\n";
        cout << "Budget Used:\n" << fixed << setprecision(1) << s.percent << "%\n";
        budgetBar(s.percent);
        cout << "\n";
    }
    cout << "Status:\n" << (s.status == "Safe" ? "Within Budget" : s.status) << "\n";
    string alert = ExpenseManager::alertMessage(s);
    if (!alert.empty()) { cout << "\n*** " << alert << " ***\n"; if (s.percent >= 100) cout << '\a'; }
    cout << "================================\n";
}

void Report::categoryReport(const ExpenseManager& m, int month, int year) {
    auto totals = m.categoryTotals(month, year);
    double sum = 0;
    for (auto& kv : totals) sum += kv.second;
    cout << "\nCATEGORY REPORT - " << Utils::monthName(month) << " " << year << "\n" << string(52, '-') << "\n";
    if (totals.empty()) { cout << "  (no expenses this month)\n"; return; }
    for (auto& kv : totals) {
        double pct = sum > 0 ? kv.second / sum * 100 : 0;
        cout << left << setw(15) << kv.first << right << setw(14) << Utils::formatMoney(kv.second)
             << setw(7) << fixed << setprecision(1) << pct << "%  " << string((int)(pct / 5), '#') << "\n";
    }
    cout << string(52, '-') << "\n" << left << setw(15) << "Total" << right << setw(14) << Utils::formatMoney(sum) << "\n";
}
