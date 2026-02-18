#include "router.hpp"
#include "parser.cpp" // Assuming Parser class is here
#include "logic_engine.cpp" // Assuming LogicEngine class is here
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

// Helper to parse the raw string into an HttpRequest object
HttpRequest parse_raw_request(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    // 1. Parse Request Line: "GET /profile HTTP/1.1"
    if (std::getline(stream, line) && !line.empty()) {
        std::stringstream ss(line);
        ss >> req.method >> req.path;
    }

    // 2. Parse Headers
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            // Trim whitespace
            value.erase(0, value.find_first_not_of(" "));
            if (!value.empty() && value.back() == '\r') value.pop_back();
            
            req.headers[key] = value;

            // Special handling for Cookies
            if (key == "Cookie") {
                std::stringstream css(value);
                std::string part;
                while (std::getline(css, part, ';')) {
                    size_t eq = part.find('=');
                    if (eq != std::string::npos) {
                        std::string ck = part.substr(0, eq);
                        ck.erase(0, ck.find_first_not_of(" "));
                        req.cookies[ck] = part.substr(eq + 1);
                    }
                }
            }
        }
    }

    // 3. Parse Body
    std::string body;
    while (std::getline(stream, line)) {
        body += line + "\n";
    }
    req.body = body;

    return req;
}

// Helper to convert HttpResponse to raw string
std::string serialize_response(const HttpResponse& res) {
    std::string output = "HTTP/1.1 " + res.status + "\r\n";
    output += "Content-Type: " + res.contentType + "\r\n";
    output += "Content-Length: " + std::to_string(res.body.size()) + "\r\n";

    for (const auto& [key, val] : res.headers) {
        output += key + ": " + val + "\r\n";
    }

    for (const auto& cookie : res.set_cookies) {
        output += "Set-Cookie: " + cookie + "\r\n";
    }

    output += "\r\n" + res.body;
    return output;
}

void handle_client(int client_fd) {
    char buffer[8192] = {0};
    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    
    if (bytes_read > 0) {
        // Parse raw text into our struct
        HttpRequest req = parse_raw_request(std::string(buffer));
        
        // Pass it to our decoupled Router
        HttpResponse res = Router::handleRequest(req);

        // Convert struct back to raw text and send
        std::string raw_res = serialize_response(res);
        send(client_fd, raw_res.c_str(), raw_res.size(), 0);
    }

    close(client_fd);
}

int main() {
    // Initialize our configuration from routes.conf
    Router::loadConfig();

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        return 1;
    }

    // Set socket options to reuse address (prevents "Address already in use" errors)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        return 1;
    }

    std::cout << "🚀 Decoupled C++ Server running on port 8080..." << std::endl;

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            // Spawn a detached thread for every request
            std::thread(handle_client, client_fd).detach();
        }
    }

    close(server_fd);
    return 0;
}