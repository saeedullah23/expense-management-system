// Utils.h - small helper functions shared by every module (validation, dates, text, console input)
#ifndef UTILS_H
#define UTILS_H
#include <string>
#include <vector>

namespace Utils {
// ---- text helpers ----
std::string trim(const std::string& s);
std::string toLower(std::string s);
std::vector<std::string> split(const std::string& s, char delim);
std::string sanitize(const std::string& s);      // removes '|' and newlines so file format never breaks
std::string jsonEscape(const std::string& s);
std::string urlDecode(const std::string& s);
std::string hashPassword(const std::string& plain); // simple FNV-1a hash (academic demo, not production security)
std::string formatMoney(double v);               // 12500 -> "Rs. 12,500"

// ---- date helpers (dates are stored internally as ISO text: YYYY-MM-DD) ----
bool isLeap(int year);
int daysInMonth(int month, int year);
bool isValidDate(int d, int m, int y);
std::string makeIso(int d, int m, int y);
bool parseDate(const std::string& in, std::string& iso); // accepts DD/MM/YYYY or YYYY-MM-DD
std::string isoToDisplay(const std::string& iso);        // YYYY-MM-DD -> DD/MM/YYYY
int dayOf(const std::string& iso);
int monthOf(const std::string& iso);
int yearOf(const std::string& iso);
std::string todayISO();
std::string monthName(int m);

// ---- number parsing ----
bool parseInt(const std::string& s, int& out);
bool parseAmount(const std::string& s, double& out);     // positive, finite, <= 1e9

// ---- safe console input (never crashes on bad input) ----
std::string readLine(const std::string& prompt);
int readInt(const std::string& prompt, int lo, int hi, bool hasDefault = false, int def = 0);
double readAmount(const std::string& prompt);
void pauseScreen();
}
#endif
