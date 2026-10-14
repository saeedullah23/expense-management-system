// FileManager.cpp
#include "FileManager.h"
#include "Utils.h"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace std;

string FileManager::dataDir = "data";

void FileManager::ensureFiles() {
    filesystem::create_directories(dataDir);
    for (const char* f : {"users.txt", "budgets.txt", "expenses.txt"}) {
        fstream fs(path(f), ios::in | ios::out | ios::app);   // fstream: opens (and creates) the file
        if (!fs) throw runtime_error(string("Cannot open ") + path(f));
    }
}

// Generic loader: reads every non-empty line and converts it with 'parse'. Corrupt lines are skipped.
template <typename T, typename Parser>
static vector<T> loadFile(const string& file, Parser parse) {
    vector<T> out;
    ifstream in(file);
    string line;
    while (getline(in, line)) {
        line = Utils::trim(line);
        if (line.empty() || line[0] == '#') continue;
        try { out.push_back(parse(line)); }
        catch (const exception&) { cerr << "Skipped bad line in " << file << "\n"; }
    }
    return out;
}

template <typename T>
static void saveFile(const string& file, const vector<T>& items) {
    ofstream out(file, ios::trunc);
    if (!out) throw runtime_error("Cannot write " + file);
    for (const auto& it : items) out << it.toLine() << "\n";
}

vector<User> FileManager::loadUsers()       { return loadFile<User>(path("users.txt"), User::fromLine); }
vector<Budget> FileManager::loadBudgets()   { return loadFile<Budget>(path("budgets.txt"), Budget::fromLine); }
vector<Expense> FileManager::loadExpenses() { return loadFile<Expense>(path("expenses.txt"), Expense::fromLine); }

void FileManager::saveUsers(const vector<User>& v)       { saveFile(path("users.txt"), v); }
void FileManager::saveBudgets(const vector<Budget>& v)   { saveFile(path("budgets.txt"), v); }
void FileManager::saveExpenses(const vector<Expense>& v) { saveFile(path("expenses.txt"), v); }

void FileManager::appendExpense(const Expense& e) {
    ofstream out(path("expenses.txt"), ios::app);
    if (!out) throw runtime_error("Cannot write expenses file");
    out << e.toLine() << "\n";
}

bool FileManager::exportCsv(const string& file, const vector<Expense>& list) {
    ofstream out(file);
    if (!out) return false;
    out << "Date,Category,Description,Payment Method,Amount\n";
    for (const auto& e : list) {
        string d = e.getDescription();
        for (auto& c : d) if (c == ',' || c == '"') c = ' ';
        out << Utils::isoToDisplay(e.getDate()) << "," << e.getCategory() << "," << d << ","
            << e.getPaymentMethod() << "," << e.getAmount() << "\n";
    }
    return true;
}
