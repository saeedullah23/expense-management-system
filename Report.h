// Report.h - prints console reports. All numbers come from ExpenseManager (no calculations duplicated here).
#ifndef REPORT_H
#define REPORT_H
#include "ExpenseManager.h"
#include <string>
#include <vector>

class Report {
public:
    static void printExpenseTable(const std::vector<Expense>& list);
    static void dailyReport(const ExpenseManager& m, const std::string& isoDate);
    static void monthlyReport(const ExpenseManager& m, int month, int year);
    static void categoryReport(const ExpenseManager& m, int month, int year);
    static void budgetBar(double percent);          // text progress bar
};
#endif
