#pragma once
#include "parser.hpp"
#include <string>
#include <functional>
#include <map>
#include <regex>

struct HttpResponse {
    std::string body;
    std::string contentType = "text/html"; // Default to HTML
    std::string status = "200 OK";
};


using HandlerFunc = std::function<HttpResponse(const HttpRequest&, const std::smatch&)>;

class Router {
public:

    struct RouteEntry{
        std::string method;
        std::regex pathRegex;
        HandlerFunc handler;
    };

    static void addRoute(const std::string& method, const std::string& path, HandlerFunc handler);
    static std::string handleRequest(const HttpRequest& request);
    static std::string readFile(const std::string& path);
private:
    static std::vector<RouteEntry> dynamicRoutes;
    static std::string getMimeType(const std::string& path);
};

#define REGISTER_ROUTE(method, path, funcName) \
    static bool _reg_##funcName = []() { \
        Router::addRoute(method, path, funcName); \
        return true; \
    }();

