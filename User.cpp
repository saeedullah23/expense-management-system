// User.cpp
#include "User.h"
#include "Utils.h"
#include <cctype>
#include <iostream>
#include <stdexcept>
using namespace std;

User::User(const string& n, const string& u, const string& h) : name(n), username(u), passwordHash(h) {}

User User::create(const string& n, const string& u, const string& plain) {
    string name = Utils::sanitize(n), user = Utils::trim(u);
    if (name.empty() || user.empty() || plain.empty())
        throw invalid_argument("Name, username and password cannot be empty.");
    if (user.size() < 3 || user.size() > 20)
        throw invalid_argument("Username must be 3-20 characters.");
    for (unsigned char c : user)
        if (!isalnum(c) && c != '_') throw invalid_argument("Username may only contain letters, digits and underscore.");
    if (plain.size() < 4) throw invalid_argument("Password must be at least 4 characters.");
    if (plain.find('|') != string::npos || plain.find(' ') != string::npos)
        throw invalid_argument("Password cannot contain spaces or '|'.");
    return User(name, user, Utils::hashPassword(plain));
}

User User::fromLine(const string& line) {
    auto f = Utils::split(line, '|');
    if (f.size() != 3) throw runtime_error("bad user record");
    return User(f[0], f[1], f[2]);
}

bool User::checkPassword(const string& plain) const { return Utils::hashPassword(plain) == passwordHash; }

string User::toLine() const { return name + "|" + username + "|" + passwordHash; }

void User::display() const { cout << name << " (" << username << ")\n"; }
