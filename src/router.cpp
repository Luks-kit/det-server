#include "router.hpp"
#include "logic_engine.hpp"
#include "logger.hpp"
#include "session_store.hpp"
#include "script_executor.hpp"
#include <fstream>
#include <iostream>

std::vector<RouteConfig> Router::configRoutes;
std::map<std::string, std::string> SessionStore::sessions;
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
            configRoutes.push_back({trim(m), trim(p), trim(s)});
        }
    }
}

HttpResponse Router::handleRequest(HttpRequest& req) {
    Logger::log(LogLevel::INFO, req.method + " " + req.path);

    if (!req.cookies.empty()) Logger::log(LogLevel::DEBUG, "Session Cookie found: " + req.cookies["sid"]);

      if (req.path.rfind("/static/", 0) == 0) { // starts with /static/
        HttpResponse res;
        res.body = readFile(req.path);
        res.contentType = getMimeType(req.path);
        Logger::log(LogLevel::INFO, "Serving static file: " + req.path);
        return res;
      }

    for (auto& route : configRoutes) {
        std::cout << "Checking against: " << route.method << " " << route.pathRegex << "\n";
        if (req.method == route.method && std::regex_match(req.path, std::regex(route.pathRegex))) {
            HttpResponse res;
            ScriptExecutor::execute(route.scriptPath, req, res);
            return res;
        }
    }
    return HttpResponse::html("404 Not Found", "404 Not Found");
}

std::string Router::getMimeType(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos == std::string::npos) return "text/plain";

    std::string ext = path.substr(dotPos);

    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "application/javascript";
    if (ext == ".png")  return "image/png";
    if (ext == ".jpg")  return "image/jpeg";
    if (ext == ".gif")  return "image/gif";
    if (ext == ".json") return "application/json";

    return "text/plain";
}

std::string Router::readFile(const std::string& path) {
    // Basic security: don't allow ".." to escape the service folder
    if (path.find("..") != std::string::npos) return "";

    std::ifstream f("service" + path);
    if (!f.is_open()) return "";
    
    std::stringstream b; 
    b << f.rdbuf();
    return b.str();
}

void Router::saveSession(std::string sid, std::string user) {
    std::lock_guard<std::mutex> lock(router_mutex);
    SessionStore::save(sid, user);
}

std::string Router::getUserFromSession(std::string sid) {
    std::lock_guard<std::mutex> lock(router_mutex);
    return SessionStore::get(sid);
}
