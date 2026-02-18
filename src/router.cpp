#include "router.hpp"
#include <fstream>
#include <iostream>

std::vector<RouteConfig> Router::configRoutes;
std::map<std::string, std::string> Router::sessionStore;
std::mutex Router::router_mutex;

void Router::loadConfig() {
    std::lock_guard<std::mutex> lock(router_mutex);
    configRoutes.clear();
    std::ifstream file("service/routes.conf");
    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string m, p, s;
        if (std::getline(ss, m, '|') && std::getline(ss, p, '|') && std::getline(ss, s, '|')) {
            configRoutes.push_back({m, p, s});
        }
    }
}

HttpResponse Router::handleRequest(HttpRequest& req) {
    for (auto& route : configRoutes) {
        if (req.method == route.method && std::regex_match(req.path, std::regex(route.pathRegex))) {
            HttpResponse res;
            // LogicEngine::execute(route.scriptPath, req, res);
            return res;
        }
    }
    return HttpResponse::html("404 Not Found", "404 Not Found");
}