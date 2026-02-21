#include "router.hpp"
#include "parser.hpp" // Assuming Parser class is here
#include "logic_engine.hpp" // Assuming LogicEngine class is here
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

constexpr size_t MAX_REQUEST_SIZE = 10 * 1024 * 1024; // 10MB limit


std::string read_full_request(int client_fd) {
    std::string raw;
    char buffer[4096];

    // 1️ Read until headers are complete
    while (raw.find("\r\n\r\n") == std::string::npos) {
        ssize_t n = read(client_fd, buffer, sizeof(buffer));
        if (n <= 0) return "";
        raw.append(buffer, n);

        if (raw.size() > MAX_REQUEST_SIZE) {
            throw std::runtime_error("Request too large");
        }
    }

    // 2️ Split headers and initial body
    size_t header_end = raw.find("\r\n\r\n");
    size_t body_start = header_end + 4;

    std::string header_part = raw.substr(0, header_end);
    std::string body_part   = raw.substr(body_start);

    // 3️ Parse Content-Length
    size_t content_length = 0;

    std::istringstream header_stream(header_part);
    std::string line;
    while (std::getline(header_stream, line)) {
        if (line.find("Content-Length:") == 0) {
            content_length = std::stoul(line.substr(15));
        }
    }

    // 4️ Read remaining body if needed
    while (body_part.size() < content_length) {
        ssize_t n = read(client_fd, buffer, sizeof(buffer));
        if (n <= 0) break;
        body_part.append(buffer, n);

        if (body_part.size() > MAX_REQUEST_SIZE) {
            throw std::runtime_error("Body too large");
        }
    }

    // 5️ Reconstruct full request
    return header_part + "\r\n\r\n" + body_part;
}

std::string url_decode(const std::string& value) {
    std::string result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') {
            result += ' ';
        } else if (value[i] == '%' && i + 2 < value.size()) {
            std::string hex = value.substr(i + 1, 2);
            char c = static_cast<char>(std::strtol(hex.c_str(), nullptr, 16));
            result += c;
            i += 2;
        } else {
            result += value[i];
        }
    }
    return result;
}



void inject_form_data(const std::string& body, ScriptContext& ctx) {
    std::stringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string val = url_decode(pair.substr(pos + 1));
            // In your script, you'll access these as form.num1, etc.
            ctx.vars["form." + key] = Value(val); 
        }
    }
}

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
                        std::string cv = part.substr(eq + 1);
                        // Trim BOTH key and value
                        ck.erase(0, ck.find_first_not_of(" "));
                        cv.erase(0, cv.find_first_not_of(" "));
                        req.cookies[ck] = cv;
                    }
                }
            }
        }
    }


  // 3. Parse Body (Safe Implementation)
    if (req.headers.count("Content-Length")) {
        try {
            size_t contentLength = std::stoul(req.headers["Content-Length"]);
            
            // Find the start of the body in the stream
            // stream.tellg() tells us where the header-parser stopped
            std::string remaining;
            std::ostringstream oss;
            oss << stream.rdbuf(); // Grab everything left in the buffer
            remaining = oss.str();

            if (remaining.size() >= contentLength) {
                req.body = remaining.substr(0, contentLength);
            } else {
                req.body = remaining;
            }
        } catch (...) {
            req.body = ""; 
        }
    }

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
    std::string raw_request = read_full_request(client_fd);

    if (raw_request.empty()) {
        close(client_fd);
        return;
    }

    HttpRequest req = parse_raw_request(raw_request);
    HttpResponse res = Router::handleRequest(req);

    std::string raw_res = serialize_response(res);
    send(client_fd, raw_res.c_str(), raw_res.size(), 0);

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
