// Record.h - abstract base class. User, Expense and Budget all INHERIT from it.
// Demonstrates inheritance + polymorphism: FileManager can treat every record the same way.
#ifndef RECORD_H
#define RECORD_H
#include <string>

class Record {
public:
    virtual ~Record() {}
    virtual std::string toLine() const = 0;   // how this record is written to a text file
    virtual void display() const = 0;         // how this record is printed on the console
};
#endif
