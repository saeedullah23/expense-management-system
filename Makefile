# Linux / macOS:  make        Windows (MinGW):  g++ -std=c++17 -O2 -o ems.exe *.cpp -lws2_32
CXX      = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
SRC      = main.cpp Utils.cpp User.cpp Expense.cpp Budget.cpp FileManager.cpp ExpenseManager.cpp Report.cpp WebServer.cpp

ems: $(SRC)
	$(CXX) $(CXXFLAGS) -o ems $(SRC)

clean:
	rm -f ems ems.exe
