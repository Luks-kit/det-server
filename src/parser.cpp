#include <string>
#include <map>
#include <sstream>
#include "parser.hpp"

// Utility function to clean up whitespace
std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, (last - first + 1));
}


std::string Parser::urlDecode(const std::string& str) {
    std::string res;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '+') res += ' ';
        else if (str[i] == '%' && i + 2 < str.length()) {
            res += (char)std::stoul(str.substr(i + 1, 2), nullptr, 16);
            i += 2;
        } else res += str[i];
    }
    return res;
}

std::map<std::string, std::string> Parser::parseForm(const std::string& body) {
    std::map<std::string, std::string> formData;
    std::stringstream ss(body);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
        size_t eq = pair.find('=');
        if (eq != std::string::npos) {
            std::string key = urlDecode(pair.substr(0, eq));
            std::string value = urlDecode(pair.substr(eq + 1));
            formData[key] = trim(value);
        }
    }

    return formData;
}
