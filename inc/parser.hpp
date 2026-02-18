#pragma once
#include <string>
#include <map>

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers; 
    std::string body;
};

class Parser {
public:
    static HttpRequest parseRequest(const std::string& rawData);
};