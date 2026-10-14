// WebServer.cpp
#include "WebServer.h"
#include "FileManager.h"
#include "Utils.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define BAD_SOCK INVALID_SOCKET
  #define CLOSE_SOCK closesocket
  #define SEND_FLAGS 0
#else
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int sock_t;
  #define BAD_SOCK (-1)
  #define CLOSE_SOCK close
  #ifdef MSG_NOSIGNAL
    #define SEND_FLAGS MSG_NOSIGNAL
  #else
    #define SEND_FLAGS 0
  #endif
#endif
using namespace std;

// ---------------- small JSON helpers (hand-built JSON keeps the project dependency-free) ----------------
static string q(const string& s) { return "\"" + Utils::jsonEscape(s) + "\""; }
static string num(double v, int prec = 2) { char b[40]; snprintf(b, sizeof b, "%.*f", prec, v); return b; }

static string expenseJson(const Expense& e) {
    return "{\"id\":" + to_string(e.getId()) + ",\"date\":" + q(e.getDate()) + ",\"displayDate\":" + q(Utils::isoToDisplay(e.getDate())) +
           ",\"amount\":" + num(e.getAmount()) + ",\"category\":" + q(e.getCategory()) + ",\"description\":" + q(e.getDescription()) +
           ",\"payment\":" + q(e.getPaymentMethod()) + "}";
}
static string expenseListJson(const vector<Expense>& v) {
    string s = "[";
    for (size_t i = 0; i < v.size(); i++) s += (i ? "," : "") + expenseJson(v[i]);
    return s + "]";
}
static string summaryJson(const MonthSummary& s) {
    return "{\"month\":" + to_string(s.month) + ",\"year\":" + to_string(s.year) + ",\"monthName\":" + q(Utils::monthName(s.month)) +
           ",\"hasBudget\":" + (s.hasBudget ? "true" : "false") + ",\"budget\":" + num(s.budget) + ",\"spent\":" + num(s.spent) +
           ",\"remaining\":" + num(s.remaining) + ",\"percent\":" + num(s.percent, 1) + ",\"status\":" + q(s.status) +
           ",\"alert\":" + q(ExpenseManager::alertMessage(s)) + "}";
}
static HttpResponse jsonOk(const string& body) { HttpResponse r; r.body = body; return r; }
static HttpResponse jsonErr(int code, const string& msg) { HttpResponse r; r.status = code; r.body = "{\"error\":" + q(msg) + "}"; return r; }

// ---------------- request parameter helpers ----------------
static string param(const HttpRequest& r, const string& k, const string& def = "") {
    auto it = r.params.find(k);
    return it == r.params.end() ? def : it->second;
}
static int paramInt(const HttpRequest& r, const string& k, int def) {
    int v;
    return Utils::parseInt(param(r, k), v) ? v : def;
}
static double paramAmount(const HttpRequest& r, const string& k) {
    double v;
    if (!Utils::parseAmount(param(r, k), v)) throw invalid_argument("Amount must be a positive number.");
    return v;
}
static void parseForm(const string& s, map<string, string>& out) {
    for (const auto& pair : Utils::split(s, '&')) {
        size_t eq = pair.find('=');
        if (eq == string::npos) continue;
        out[Utils::urlDecode(pair.substr(0, eq))] = Utils::urlDecode(pair.substr(eq + 1));
    }
}

WebServer::WebServer(ExpenseManager& m, int p, const string& root) : mgr(m), port(p), webRoot(root) {}

string WebServer::newToken() {
    static mt19937_64 rng((random_device())());
    char b[40];
    snprintf(b, sizeof b, "%016llx%016llx", (unsigned long long)rng(), (unsigned long long)rng());
    return b;
}

// ---------------- static files ----------------
HttpResponse WebServer::staticFile(const string& p) {
    string path = (p == "/") ? "/index.html" : p;
    if (path.find("..") != string::npos) return jsonErr(403, "Forbidden");
    ifstream in(webRoot + path, ios::binary);
    if (!in) return jsonErr(404, "Not found");
    stringstream ss;
    ss << in.rdbuf();
    HttpResponse r;
    r.body = ss.str();
    if (path.size() > 5 && path.substr(path.size() - 5) == ".html") r.contentType = "text/html; charset=utf-8";
    else if (path.size() > 4 && path.substr(path.size() - 4) == ".css") r.contentType = "text/css; charset=utf-8";
    else if (path.size() > 3 && path.substr(path.size() - 3) == ".js") r.contentType = "application/javascript; charset=utf-8";
    else if (path.size() > 4 && path.substr(path.size() - 4) == ".svg") r.contentType = "image/svg+xml";
    else r.contentType = "application/octet-stream";
    return r;
}

// ---------------- JSON API ----------------
HttpResponse WebServer::api(HttpRequest& req) {
    const string& p = req.path;
    bool post = (req.method == "POST");

    // --- public routes ---
    if (p == "/api/register" && post) {
        mgr.registerUser(param(req, "name"), param(req, "username"), param(req, "password"));
        return jsonOk("{\"message\":\"Registration successful!\"}");
    }
    if (p == "/api/login" && post) {
        mgr.logout();
        if (!mgr.login(param(req, "username"), param(req, "password"))) return jsonErr(401, "Invalid username or password.");
        string token = newToken();
        sessions[token] = mgr.currentUsername();
        string name = mgr.currentName();
        mgr.logout();
        return jsonOk("{\"token\":" + q(token) + ",\"name\":" + q(name) + ",\"message\":" + q("Login successful! Welcome " + name + "!") + "}");
    }

    // --- everything below needs a valid session token ---
    auto h = req.headers.find("x-token");
    if (h == req.headers.end() || !sessions.count(h->second)) return jsonErr(401, "Please login first.");
    string token = h->second;
    mgr.setCurrentUser(sessions[token]);          // from now on ExpenseManager only sees THIS user's data

    string today = Utils::todayISO();
    int month = paramInt(req, "month", Utils::monthOf(today));
    int year = paramInt(req, "year", Utils::yearOf(today));

    if (p == "/api/logout" && post) { sessions.erase(token); mgr.logout(); return jsonOk("{\"message\":\"Logged out.\"}"); }

    if (p == "/api/meta") {
        string s = "{\"categories\":[";
        const auto& c = Expense::categories();
        for (size_t i = 0; i < c.size(); i++) s += (i ? "," : "") + q(c[i]);
        s += "],\"payments\":[";
        const auto& m = Expense::paymentMethods();
        for (size_t i = 0; i < m.size(); i++) s += (i ? "," : "") + q(m[i]);
        return jsonOk(s + "],\"name\":" + q(mgr.currentName()) + ",\"today\":" + q(today) + "}");
    }

    if (p == "/api/dashboard") {
        auto todays = mgr.dayExpenses(today);
        auto recent = mgr.userExpenses();
        ExpenseManager::sortExpenses(recent, SortField::Date, false);
        if (recent.size() > 6) recent.resize(6);
        return jsonOk("{\"name\":" + q(mgr.currentName()) + ",\"summary\":" + summaryJson(mgr.summary(month, year)) +
                      ",\"todayTotal\":" + num(ExpenseManager::total(todays)) + ",\"todayCount\":" + to_string(todays.size()) +
                      ",\"recent\":" + expenseListJson(recent) + "}");
    }

    if (p == "/api/expenses") {
        auto list = ExpenseManager::search(mgr.monthExpenses(paramInt(req, "month", 0), year), param(req, "q"));
        string sort = param(req, "sort", "date");
        SortField f = sort == "amount" ? SortField::Amount : sort == "category" ? SortField::Category : SortField::Date;
        ExpenseManager::sortExpenses(list, f, param(req, "order", "desc") == "asc");
        return jsonOk("{\"total\":" + num(ExpenseManager::total(list)) + ",\"expenses\":" + expenseListJson(list) + "}");
    }

    if (p == "/api/expense" && post) {
        int id = mgr.addExpense(param(req, "date"), paramAmount(req, "amount"), param(req, "category"), param(req, "description"), param(req, "payment"));
        string d; Utils::parseDate(param(req, "date"), d);
        return jsonOk("{\"id\":" + to_string(id) + ",\"message\":\"Expense added successfully.\",\"summary\":" +
                      summaryJson(mgr.summary(Utils::monthOf(d), Utils::yearOf(d))) + "}");
    }
    if (p == "/api/expense/update" && post) {
        mgr.editExpense(paramInt(req, "id", -1), param(req, "date"), paramAmount(req, "amount"), param(req, "category"), param(req, "description"), param(req, "payment"));
        return jsonOk("{\"message\":\"Expense updated successfully.\"}");
    }
    if (p == "/api/expense/delete" && post) {
        mgr.deleteExpense(paramInt(req, "id", -1));
        return jsonOk("{\"message\":\"Expense deleted successfully.\"}");
    }

    if (p == "/api/budget" && post) {
        mgr.setBudget(month, year, paramAmount(req, "amount"));
        return jsonOk("{\"message\":\"Budget saved.\",\"summary\":" + summaryJson(mgr.summary(month, year)) + "}");
    }
    if (p == "/api/summary") return jsonOk(summaryJson(mgr.summary(month, year)));

    if (p == "/api/categories") {
        auto t = mgr.categoryTotals(month, year);
        double sum = 0;
        for (auto& kv : t) sum += kv.second;
        string s = "{\"total\":" + num(sum) + ",\"items\":[";
        bool first = true;
        for (auto& kv : t) {
            s += string(first ? "" : ",") + "{\"category\":" + q(kv.first) + ",\"amount\":" + num(kv.second) + ",\"percent\":" + num(sum > 0 ? kv.second / sum * 100 : 0, 1) + "}";
            first = false;
        }
        return jsonOk(s + "]}");
    }
    if (p == "/api/daily-chart") {
        auto d = mgr.dailyTotals(month, year);
        string s = "{\"days\":[";
        for (size_t i = 0; i < d.size(); i++) s += (i ? "," : "") + num(d[i]);
        return jsonOk(s + "]}");
    }
    if (p == "/api/daily") {
        string iso;
        if (!Utils::parseDate(param(req, "date", today), iso)) return jsonErr(400, "Invalid date.");
        auto list = mgr.dayExpenses(iso);
        return jsonOk("{\"date\":" + q(Utils::isoToDisplay(iso)) + ",\"total\":" + num(ExpenseManager::total(list)) +
                      ",\"count\":" + to_string(list.size()) + ",\"expenses\":" + expenseListJson(list) + "}");
    }
    if (p == "/api/history") {
        auto h2 = mgr.history();
        string s = "{\"months\":[";
        for (size_t i = 0; i < h2.size(); i++) s += (i ? "," : "") + summaryJson(h2[i]);
        return jsonOk(s + "]}");
    }
    if (p == "/api/export") {
        string file = mgr.exportMonth(month, year);
        ifstream in(file, ios::binary);
        stringstream ss;
        ss << in.rdbuf();
        HttpResponse r;
        r.contentType = "text/csv; charset=utf-8";
        r.body = ss.str();
        return r;
    }
    return jsonErr(404, "Unknown API route.");
}

HttpResponse WebServer::handle(HttpRequest& req) {
    try {
        if (req.path.rfind("/api/", 0) == 0) return api(req);
        if (req.method == "GET") return staticFile(req.path);
        return jsonErr(405, "Method not allowed.");
    } catch (const invalid_argument& e) {
        return jsonErr(400, e.what());
    } catch (const exception& e) {
        return jsonErr(400, e.what());
    }
}

// ---------------- socket loop ----------------
static void sendAll(sock_t c, const string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        int n = (int)send(c, data.data() + sent, (int)(data.size() - sent), SEND_FLAGS);
        if (n <= 0) return;
        sent += n;
    }
}

static const char* statusText(int s) {
    switch (s) { case 200: return "OK"; case 400: return "Bad Request"; case 401: return "Unauthorized";
                 case 403: return "Forbidden"; case 404: return "Not Found"; default: return "Error"; }
}

void WebServer::run() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) throw runtime_error("WSAStartup failed");
#endif
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == BAD_SOCK) throw runtime_error("Cannot create socket");
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);          // localhost only: not reachable from other computers
    if (bind(srv, (sockaddr*)&addr, sizeof addr) < 0 || listen(srv, 16) < 0)
        throw runtime_error("Cannot listen on port " + to_string(port) + " (is it already in use?)");
    cout << "Web interface running at http://localhost:" << port << "\nPress Ctrl+C to stop.\n";

    while (true) {
        sock_t c = accept(srv, nullptr, nullptr);
        if (c == BAD_SOCK) continue;

        // 1) read the request (headers first, then the body if Content-Length says so)
        string raw;
        char buf[4096];
        size_t headEnd;
        bool ok = true;
        while ((headEnd = raw.find("\r\n\r\n")) == string::npos) {
            int n = (int)recv(c, buf, sizeof buf, 0);
            if (n <= 0 || raw.size() > (1u << 20)) { ok = false; break; }
            raw.append(buf, n);
        }
        if (!ok) { CLOSE_SOCK(c); continue; }

        HttpRequest req;
        istringstream hs(raw.substr(0, headEnd));
        string line, target;
        getline(hs, line);
        istringstream rl(line);
        rl >> req.method >> target;
        while (getline(hs, line)) {
            line = Utils::trim(line);
            size_t colon = line.find(':');
            if (colon != string::npos) req.headers[Utils::toLower(line.substr(0, colon))] = Utils::trim(line.substr(colon + 1));
        }
        size_t need = 0;
        if (req.headers.count("content-length")) need = (size_t)atoi(req.headers["content-length"].c_str());
        while (raw.size() < headEnd + 4 + need && need < (1u << 20)) {
            int n = (int)recv(c, buf, sizeof buf, 0);
            if (n <= 0) break;
            raw.append(buf, n);
        }
        string body = raw.substr(headEnd + 4);

        size_t qm = target.find('?');
        req.path = Utils::urlDecode(target.substr(0, qm));
        if (qm != string::npos) parseForm(target.substr(qm + 1), req.params);
        parseForm(body, req.params);

        // 2) handle and reply
        HttpResponse res = handle(req);
        string out = "HTTP/1.1 " + to_string(res.status) + " " + statusText(res.status) + "\r\nContent-Type: " + res.contentType +
                     "\r\nContent-Length: " + to_string(res.body.size()) + "\r\nCache-Control: no-store\r\nConnection: close\r\n" +
                     res.extraHeaders + "\r\n" + res.body;
        sendAll(c, out);
        CLOSE_SOCK(c);
    }
}
