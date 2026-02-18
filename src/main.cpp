#include "parser.hpp"
#include "router.hpp"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread> // Required for multi-threading
#include <mutex>
#include <arpa/inet.h>

std::mutex cout_mutex;

void log_request(const std::string& ip, const HttpRequest& req, int status){
    std::lock_guard<std::mutex> lock(cout_mutex);
    std::cout << "[" << ip << "] " << req.method 
    << " " << req.path << " - Status: " << status << std::endl;
}

// This function runs in its own thread for every single client
 
void handle_client(int client_fd, std::string client_ip) {
    std::string raw_request;
    char buffer[4096];
    ssize_t bytes_read;

    // 1. Read initial chunk (usually contains all headers + start of body)
    bytes_read = read(client_fd, buffer, sizeof(buffer));
    if (bytes_read <= 0) {
        close(client_fd);
        return;
    }
    raw_request.append(buffer, bytes_read);

    // 2. Initial parse to see what we're dealing with
    HttpRequest req = Parser::parseRequest(raw_request);

    // 3. Check if we have a body to wait for (Mandatory for POST)
    if (req.headers.count("Content-Length")) {
        size_t expected_total_body = std::stoul(req.headers["Content-Length"]);
        
        // Keep reading until the body we've parsed matches the Content-Length
        while (req.body.size() < expected_total_body) {
            bytes_read = read(client_fd, buffer, sizeof(buffer));
            if (bytes_read <= 0) break; // Connection closed or error
            
            req.body.append(buffer, bytes_read);
        }
    }

    // 4. Process the fully assembled request
    std::string response = Router::handleRequest(req);

    // 5. Logging and Sending
    try {
        // Extract status safely (e.g., "HTTP/1.1 200 OK" -> 200)
        int status = std::stoi(response.substr(9, 3));
        log_request(client_ip, req, status);
    } catch (...) {
        log_request(client_ip, req, 0); // Fallback for malformed responses
    }

    send(client_fd, response.c_str(), response.size(), 0);
    close(client_fd);
}


int main() {



    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Allow port reuse (prevents "Address already in use" errors on restart)
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = {AF_INET, htons(8080), INADDR_ANY};
    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    std::cout << "Multi-threaded server running on port 8080..." << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) continue;

        std::string ip_str = inet_ntoa(client_addr.sin_addr);
        // Create a thread and detach it so it cleans up after itself
        std::thread(handle_client, client_fd, ip_str).detach();
    }

    return 0;
}