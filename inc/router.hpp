#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <regex>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> cookies;
};

struct HttpResponse {
    std::string status = "200 OK";
    std::string contentType = "text/html";
    std::string body;
    std::map<std::string, std::string> headers;
    std::vector<std::string> set_cookies;

    void add_cookie(const std::string& name, const std::string& value, const std::string& options = "Path=/; HttpOnly") {
        set_cookies.push_back(name + "=" + value + "; " + options);
    }

    static HttpResponse html(const std::string& b, const std::string& s = "200 OK") {
        HttpResponse res; res.body = b; res.status = s; return res;
    }
};

struct RouteConfig {
    std::string method;
    std::string pathRegex;
    std::string scriptPath;
};

class Router {
public:
    static void loadConfig();
    static HttpResponse handleRequest(HttpRequest& req);
    static std::string readFile(const std::string& path);
    static void saveSession(std::string sid, std::string user);
    static std::string getUserFromSession(std::string sid);

private:
    static std::vector<RouteConfig> configRoutes;
    static std::map<std::string, std::string> sessionStore;
    static std::mutex router_mutex;
};

