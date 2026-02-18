#include "router.hpp"
#include <fstream>
#include <sstream>

std::vector<Router::RouteEntry> Router::dynamicRoutes;


void Router::addRoute(
    const std::string& method, 
    const std::string& pathPattern, 
    HandlerFunc handler
    ) {
    
    std::string pattern = std::regex_replace(pathPattern, std::regex("\\{[^/]+\\}"), "([^/]+)");
    dynamicRoutes.push_back({method, std::regex(pattern), handler});
}

std::string Router::readFile(const std::string& path) {
    std::ifstream file("service" + path, std::ios::binary);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string Router::getMimeType(const std::string& path){
    static std::map<std::string, std::string> mimeTypes = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".txt", "text/plain"}
    };
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = path.substr(dotPos);
        if (mimeTypes.count(ext)) {
            return mimeTypes[ext];
        } else {
            return "application/octet-stream";
        }
}
    return "text/plain";
}

std::string Router::handleRequest(const HttpRequest& request) {
    // 1. Check Dynamic Regex Routes
    for (const auto& route : dynamicRoutes) {
        std::smatch matches;
        if (request.method == route.method && 
            std::regex_match(request.path, matches, route.pathRegex)) {
            
            // Note: 'matches' contains the captured groups (like the ID in /user/(\d+))
            // For now, we just call the handler.
            HttpResponse res = route.handler(request, matches);
            
            return "HTTP/1.1 " + res.status + "\r\n" +
                   "Content-Type: " + res.contentType + "\r\n" +
                   "Content-Length: " + std::to_string(res.body.size()) + "\r\n" +
                   "\r\n" + res.body;
        }
    }

    // 2. Fallback: Static Files (Same as before)
    std::string content = readFile(request.path);
    if (!content.empty()) {
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: " + getMimeType(request.path) + "\r\n" +
               "Content-Length: " + std::to_string(content.size()) + "\r\n" +
               "\r\n" + content;
    }

    return "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
}
