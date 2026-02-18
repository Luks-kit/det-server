#include "parser.hpp"
#include <sstream>


HttpRequest Parser::parseRequest(const std::string& rawData) {
    HttpRequest req;
    
    size_t header_end = rawData.find("\r\n\r\n");
    if (header_end == std::string::npos) return req;

    // Extract headers string and body string
    std::string headers_part = rawData.substr(0, header_end);
    req.body = rawData.substr(header_end + 4); 

    std::istringstream stream(headers_part);
    std::string line;
    
    std::getline(stream, line);
    std::istringstream request_line(line);
    request_line >> req.method >> req.path;

    // Parse individual header lines
    while (std::getline(stream, line) && line != "\r") {
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            
            // Trim whitespace and carriage returns
            value.erase(0, value.find_first_not_of(" "));
            if (!value.empty() && value.back() == '\r') value.pop_back();
            
            req.headers[key] = value;
        }
    }

    return req;
}