// User.h - a registered person. Password is stored only as a hash.
#ifndef USER_H
#define USER_H
#include "Record.h"

class User : public Record {
private:
    std::string name;
    std::string username;
    std::string passwordHash;
public:
    User() {}
    User(const std::string& name, const std::string& username, const std::string& hash);

    // Validates input and builds a new User (throws std::invalid_argument on bad input)
    static User create(const std::string& name, const std::string& username, const std::string& plainPassword);
    static User fromLine(const std::string& line);

    const std::string& getName() const { return name; }
    const std::string& getUsername() const { return username; }
    bool checkPassword(const std::string& plain) const;   // used by login

    std::string toLine() const override;
    void display() const override;
};
#endif
