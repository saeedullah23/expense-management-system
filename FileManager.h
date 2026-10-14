// FileManager.h - ALL disk access lives here (ifstream / ofstream / fstream).
#ifndef FILEMANAGER_H
#define FILEMANAGER_H
#include "Budget.h"
#include "Expense.h"
#include "User.h"
#include <string>
#include <vector>

class FileManager {
public:
    static std::string dataDir;                 // default "data"

    static void ensureFiles();                  // creates data/ and empty files if missing
    static std::vector<User> loadUsers();
    static void saveUsers(const std::vector<User>& users);
    static std::vector<Budget> loadBudgets();
    static void saveBudgets(const std::vector<Budget>& budgets);
    static std::vector<Expense> loadExpenses();
    static void saveExpenses(const std::vector<Expense>& expenses);   // rewrite whole file (edit/delete)
    static void appendExpense(const Expense& e);                      // fast path for "add"
    static bool exportCsv(const std::string& path, const std::vector<Expense>& list);
private:
    static std::string path(const std::string& file) { return dataDir + "/" + file; }
};
#endif
