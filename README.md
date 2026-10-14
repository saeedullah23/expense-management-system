# Daily Expense & Monthly Budget Management System (C++)

A C++17 academic project. **All logic is in C++**: authentication, budgets, expenses, calculations, alerts,
searching, sorting, reports and file handling. The browser interface is only a display layer: it asks the C++
program (through a small C++ HTTP server) for data and shows it.

## Project structure

```
ExpenseManagementSystem/
├── main.cpp              console menu + "--web" switch
├── Record.h              abstract base class (inheritance / polymorphism)
├── User.h / .cpp         user account
├── Expense.h / .cpp      one expense
├── Budget.h / .cpp       monthly budget
├── ExpenseManager.h/.cpp business logic: login, CRUD, search, sort, totals, alerts, history, export
├── Report.h / .cpp       console reports (daily / monthly / category)
├── FileManager.h / .cpp  all file I/O (ifstream, ofstream, fstream)
├── Utils.h / .cpp        validation, dates, money format, safe console input
├── WebServer.h / .cpp    tiny C++ HTTP server + JSON API (sockets, no libraries)
├── Makefile
├── data/                 users.txt  budgets.txt  expenses.txt  (sample data included)
└── web/                  index.html  style.css  script.js
```

The web UI is a single page (`index.html`) that switches between Login, Register, Dashboard, Expenses, Budget,
Reports and History, instead of one HTML file per page.

## How to compile

**Linux / macOS**
```
make
```
or `g++ -std=c++17 -O2 -o ems *.cpp`

**Windows (MinGW g++, or VS Code with MinGW)**
```
g++ -std=c++17 -O2 -o ems.exe *.cpp -lws2_32
```
(`-lws2_32` is needed only for the web server.)

**Visual Studio:** add all `.cpp` files, set the language standard to C++17. The Winsock library is linked automatically
through the includes in the project settings; if you get a linker error add `ws2_32.lib`.

## How to run

Always start the program **from the project folder** (so `data/` and `web/` are found).

| Mode | Command | Notes |
|------|---------|-------|
| Console (main version) | `./ems` (Windows: `ems.exe`) | full menu, all features |
| Web interface | `./ems --web` then open **http://localhost:8080** | optional port: `./ems --web 9000` |

The web server only listens on your own computer (127.0.0.1). The console and web modes use the same data files.

### Sample accounts

| Username | Password | What to look at |
|----------|----------|-----------------|
| `ali123` | `pass123` | Jan-Oct 2026, with Safe / Warning / Near Limit / Exceeded months. October 2026 is at 85% so the 80% warning shows. |
| `ahmed123` | `pass123` | Separate data for Sep-Oct 2026, to show that users never see each other's records. |

Log in as one, then the other: the expenses, budgets, reports and history are completely different.

## Data files

Plain text, one record per line, fields separated by `|`:

```
users.txt     Name|username|passwordHash
budgets.txt   username|month|year|amount
expenses.txt  id|username|date(YYYY-MM-DD)|amount|category|description|paymentMethod
```
Every budget and expense carries the owner's `username`. `ExpenseManager` only ever returns records whose
username equals the logged-in user, which is how multi-user separation works. Passwords are stored as a hash
(FNV-1a with a prefix), which is fine for a college project; a real product would use bcrypt/argon2.

## Budget alert levels (done in C++, `ExpenseManager::statusFor / alertMessage`)

| Used | Status | Message |
|------|--------|---------|
| below 80% | Safe (green) | none |
| 80% to below 90% | Warning (yellow) | WARNING: You have used 80% of your monthly budget. |
| 90% to below 100% | Near Limit (orange) | WARNING: You have used 90% of your monthly budget. |
| exactly 100% | Limit Reached (red) | ALERT: Your monthly budget has been reached! |
| above 100% | Exceeded (red) | ALERT: Your monthly budget has been exceeded! |

The console also sounds the terminal beep (`'\a'`) at 100% and above.

---

# Explanation of the major classes

| Class | Responsibility | Key members |
|-------|----------------|-------------|
| `Record` | Abstract base class. Forces every record to know how to save itself and print itself. | `virtual toLine() = 0`, `virtual display() = 0` |
| `User` | One account. Stores name, username and password **hash**. | `create()` (validates), `checkPassword()`, `fromLine()` |
| `Expense` | One expense. Private fields, validating setters. | `setAmount()` rejects negative/zero, `setDate()` rejects invalid dates, `categories()`, `paymentMethods()` |
| `Budget` | Budget of one user for one month/year. | `calculateRemaining()`, `calculatePercentUsed()` |
| `FileManager` | The only class that touches files. | `loadExpenses()`, `saveExpenses()`, `appendExpense()`, `exportCsv()` |
| `ExpenseManager` | Business logic. Owns `vector<User>`, `vector<Budget>`, `vector<Expense>`. | `registerUser()`, `login()`, `addExpense()`, `editExpense()`, `deleteExpense()`, `search()`, `sortExpenses()`, `summary()`, `history()` |
| `Report` | Formats console reports using numbers from `ExpenseManager`. | `dailyReport()`, `monthlyReport()`, `categoryReport()` |
| `WebServer` | Accepts HTTP requests, calls `ExpenseManager`, returns JSON. | `run()`, `api()` |
| `Utils` (namespace) | Reusable helpers. | `parseDate()`, `parseAmount()`, `readInt()`, `formatMoney()` |

# OOP concepts used (and where)

- **Classes and objects**: `User`, `Expense`, `Budget`, `ExpenseManager`, `Report`, `FileManager`, `WebServer`.
- **Encapsulation**: every data member is `private`; the outside world uses getters and validating setters. An
  `Expense` with a negative amount can never exist because `setAmount()` throws.
- **Inheritance**: `User`, `Expense`, `Budget` all inherit from `Record`.
- **Polymorphism**: each subclass overrides `toLine()` and `display()` (`override` keyword). The file code in
  `FileManager` saves any record type through the same `toLine()` call.
- **Abstraction**: `Record` is an abstract class (pure virtual functions); `ExpenseManager` hides all the file and
  vector details behind simple functions like `addExpense()`.
- **Static members**: `User::create`, `Expense::categories`, `ExpenseManager::search` do not need an object.
- **Templates**: `loadFile<T>` / `saveFile<T>` in `FileManager.cpp` are generic functions that work for users, budgets and expenses.
- **STL containers and algorithms**: `vector`, `map` (category totals), `set` (months for history), `stable_sort` with a lambda.
- **Exception handling**: `throw invalid_argument(...)` in setters, `try/catch` in the menus and the web server so bad input never crashes the program.
- **Modular programming**: each `.h/.cpp` pair has one job; `main.cpp` only draws menus.

# Explanation of file handling

- `ifstream` reads files line by line (`loadFile`). Each line is split on `|` and turned into an object. A corrupt line is skipped, not fatal.
- `ofstream` with `ios::trunc` rewrites a whole file (`saveFile`), used after edit, delete, new user or budget change.
- `ofstream` with `ios::app` appends one line (`appendExpense`), used for a quick "add expense".
- `fstream` with `ios::in | ios::out | ios::app` (`ensureFiles`) opens a file and creates it if it does not exist yet.
- Data survives closing the program because each change is written to disk immediately.
- Monthly export writes `data/export_<username>_<yyyy-mm>.csv`, which opens in Excel.

# Algorithms

- **Searching**: `ExpenseManager::search` lower-cases the keyword and checks whether it appears in the date, category, description or payment method (linear search).
- **Sorting**: `stable_sort` with a lambda comparator; fields Date / Amount / Category, ascending or descending, with the expense ID as a tie-breaker so results are always in the same order.
- **Calculations**: `remaining = budget - spent`, `percent = spent / budget * 100`, category totals in a `map<string,double>`, daily totals in a `vector<double>` indexed by day of month.

---

# Viva questions and answers

**1. Why did you use C++ for this project?**
C++ holds all the logic: classes, file I/O, calculations and validation. The HTML/CSS/JS part only displays what the C++ program returns.

**2. How does the web page talk to C++?**
The program started with `--web` runs a small HTTP server written with sockets. The browser sends requests such as `/api/dashboard`; C++ calculates everything and replies with JSON; JavaScript only draws it.

**3. What is encapsulation? Show an example.**
Keeping data private and exposing controlled access. `Expense::amount` is private and can only be changed through `setAmount()`, which throws an exception for zero or negative values.

**4. Where did you use inheritance and polymorphism?**
`User`, `Expense` and `Budget` inherit from the abstract class `Record` and override `toLine()` and `display()`. Code that has a `Record` can save or print it without knowing its exact type.

**5. What is an abstract class?**
A class with at least one pure virtual function (`= 0`). It cannot be instantiated. `Record` is one.

**6. What does `override` do?**
It tells the compiler that the function must override a virtual function of the base class, so a typo in the signature becomes a compile error.

**7. How do you keep each user's data separate?**
Every budget and expense stores the owner's username. `ExpenseManager` filters by `currentUser` on every read, and edit/delete only find records owned by that user. The web server maps each login token to one username.

**8. How are passwords stored?**
As a hash, not plain text. The hash is a simple FNV-1a for the project; real systems should use bcrypt or argon2 with a random salt.

**9. How is data stored permanently?**
In text files in `data/` using `ifstream`, `ofstream` and `fstream`. The files are loaded when the program starts and saved after every change.

**10. Difference between `ofstream` with `ios::app` and `ios::trunc`?**
`app` adds to the end of the file; `trunc` empties the file first, so the whole file is rewritten. We append for "add" and rewrite for "edit/delete".

**11. How is the budget alert calculated?**
`percent = spent / budget * 100`. Then `statusFor()` compares it with 80, 90 and 100 and `alertMessage()` returns the matching text.

**12. How do you prevent crashes on wrong input?**
Console input is read as text and checked (`parseInt`, `parseAmount`, `parseDate`) in a loop until valid. Setters throw exceptions on bad values, and menus catch them with `try/catch`.

**13. Which STL containers did you use and why?**
`vector` for lists of records, `map` for category totals (keys sorted automatically), `set` of (year, month) pairs to build the history without duplicates.

**14. What is a template? Where is it used?**
A function written once for any type. `loadFile<T>` and `saveFile<T>` in `FileManager.cpp` work for users, budgets and expenses.

**15. How does sorting work?**
`stable_sort(begin, end, lambda)`. The lambda compares two expenses by the chosen field and ID as tie-breaker, then reverses the answer for descending order.

**16. Why is the date stored as YYYY-MM-DD?**
Text in that format sorts in the same order as the dates, and the month and year are easy to extract with `substr`.

**17. What is the time complexity of searching and sorting?**
Search is O(n) over the user's expenses; sorting is O(n log n).

**18. What happens if the data file has a damaged line?**
`loadFile` catches the exception for that line, prints a message and continues with the rest.

**19. How could the project be improved?**
Use a real database (SQLite), proper password hashing with salt, a JSON library, threads for the web server, recurring expenses, and unit tests.
