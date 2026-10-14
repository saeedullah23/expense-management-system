// Utils.cpp
#include "Utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
using namespace std;

namespace Utils {

string trim(const string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

string toLower(string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

vector<string> split(const string& s, char delim) {
    vector<string> out;
    string cur;
    for (char c : s) {
        if (c == delim) { out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    out.push_back(cur);
    return out;
}

string sanitize(const string& s) {
    string r = s;
    for (auto& c : r) if (c == '|') c = '/'; else if (c == '\n' || c == '\r' || c == '\t') c = ' ';
    return trim(r);
}

string jsonEscape(const string& s) {
    string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += (char)c;
        }
    }
    return o;
}

string urlDecode(const string& s) {
    string o;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '+') o += ' ';
        else if (s[i] == '%' && i + 2 < s.size() + 0 && isxdigit((unsigned char)s[i + 1]) && isxdigit((unsigned char)s[i + 2])) {
            o += (char)strtol(s.substr(i + 1, 2).c_str(), nullptr, 16);
            i += 2;
        } else o += s[i];
    }
    return o;
}

string hashPassword(const string& plain) {
    unsigned long long h = 1469598103934665603ULL;      // FNV-1a offset basis
    string salted = "ems$" + plain;
    for (unsigned char c : salted) { h ^= c; h *= 1099511628211ULL; }
    char buf[32];
    snprintf(buf, sizeof buf, "%016llx", h);
    return buf;
}

string formatMoney(double v) {
    char buf[64];
    snprintf(buf, sizeof buf, "%.2f", fabs(v));
    string s = buf;
    size_t dot = s.find('.');
    string ip = s.substr(0, dot), fp = s.substr(dot);
    if (fp == ".00") fp = "";
    string out;
    int count = 0;
    for (int i = (int)ip.size() - 1; i >= 0; --i) {
        out.insert(out.begin(), ip[i]);
        if (++count % 3 == 0 && i > 0) out.insert(out.begin(), ',');
    }
    return string(v < 0 ? "-" : "") + "Rs. " + out + fp;
}

bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int daysInMonth(int m, int y) {
    static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (m < 1 || m > 12) return 0;
    return (m == 2 && isLeap(y)) ? 29 : d[m - 1];
}

bool isValidDate(int d, int m, int y) {
    return y >= 2000 && y <= 2100 && m >= 1 && m <= 12 && d >= 1 && d <= daysInMonth(m, y);
}

string makeIso(int d, int m, int y) {
    char b[64];
    snprintf(b, sizeof b, "%04d-%02d-%02d", y, m, d);
    return b;
}

static bool allDigits(const string& s) {
    return !s.empty() && all_of(s.begin(), s.end(), [](unsigned char c) { return isdigit(c) != 0; });
}

bool parseDate(const string& in, string& iso) {
    string s = trim(in);
    char delim = (s.find('/') != string::npos) ? '/' : '-';
    vector<string> p = split(s, delim);
    if (p.size() != 3) return false;
    for (auto& x : p) if (!allDigits(x) || x.size() > 4) return false;
    int d, m, y;
    if (delim == '/') { d = stoi(p[0]); m = stoi(p[1]); y = stoi(p[2]); }
    else              { y = stoi(p[0]); m = stoi(p[1]); d = stoi(p[2]); }
    if (!isValidDate(d, m, y)) return false;
    iso = makeIso(d, m, y);
    return true;
}

string isoToDisplay(const string& iso) {
    if (iso.size() != 10) return iso;
    return iso.substr(8, 2) + "/" + iso.substr(5, 2) + "/" + iso.substr(0, 4);
}
int dayOf(const string& iso)   { return iso.size() == 10 ? atoi(iso.substr(8, 2).c_str()) : 0; }
int monthOf(const string& iso) { return iso.size() == 10 ? atoi(iso.substr(5, 2).c_str()) : 0; }
int yearOf(const string& iso)  { return iso.size() == 10 ? atoi(iso.substr(0, 4).c_str()) : 0; }

string todayISO() {
    time_t t = time(nullptr);
    tm* lt = localtime(&t);
    return makeIso(lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900);
}

string monthName(int m) {
    static const char* n[] = {"January", "February", "March", "April", "May", "June", "July",
                              "August", "September", "October", "November", "December"};
    return (m >= 1 && m <= 12) ? n[m - 1] : "?";
}

bool parseInt(const string& s, int& out) {
    string t = trim(s);
    if (!allDigits(t) || t.size() > 9) return false;
    out = stoi(t);
    return true;
}

bool parseAmount(const string& s, double& out) {
    string t;
    for (char c : trim(s)) if (c != ',') t += c;      // allow 1,500
    if (t.empty()) return false;
    char* end = nullptr;
    double v = strtod(t.c_str(), &end);
    if (*end != '\0' || !isfinite(v) || v <= 0 || v > 1e9) return false;
    out = v;
    return true;
}

string readLine(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) { cout << "\n"; exit(0); }   // end of input -> leave cleanly
    return trim(s);
}

int readInt(const string& prompt, int lo, int hi, bool hasDefault, int def) {
    while (true) {
        string s = readLine(prompt);
        if (s.empty() && hasDefault) return def;
        int v;
        if (parseInt(s, v) && v >= lo && v <= hi) return v;
        cout << "Invalid input. Please try again.\n";
    }
}

double readAmount(const string& prompt) {
    while (true) {
        double v;
        if (parseAmount(readLine(prompt), v)) return v;
        cout << "Invalid input. Please try again.\n";
    }
}

void pauseScreen() { readLine("\nPress Enter to continue..."); }

} // namespace Utils
