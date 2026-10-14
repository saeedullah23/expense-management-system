// WebServer.h - a tiny HTTP server written in plain C++ (sockets only, no libraries).
// It serves the files in web/ and exposes a JSON API that calls ExpenseManager.
// All business logic stays in C++; the browser only displays what the API returns.
#ifndef WEBSERVER_H
#define WEBSERVER_H
#include "ExpenseManager.h"
#include <map>
#include <string>

struct HttpRequest {
    std::string method, path;
    std::map<std::string, std::string> params;    // query string + form body merged
    std::map<std::string, std::string> headers;   // keys in lower case
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
    std::string extraHeaders;
};

class WebServer {
private:
    ExpenseManager& mgr;
    int port;
    std::string webRoot;
    std::map<std::string, std::string> sessions;  // token -> username

    HttpResponse handle(HttpRequest& req);
    HttpResponse api(HttpRequest& req);
    HttpResponse staticFile(const std::string& path);
    std::string newToken();
public:
    WebServer(ExpenseManager& m, int port, const std::string& root = "web");
    void run();                                   // blocks forever
};
#endif
