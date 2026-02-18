#include <string>
#include <map>
#include <sstream>

class Parser {
public:
    static std::string urlDecode(const std::string& str) {
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

    static std::map<std::string, std::string> parseForm(const std::string& body) {
        std::map<std::string, std::string> data;
        std::stringstream ss(body);
        std::string pair;
        while (std::getline(ss, pair, '&')) {
            size_t eq = pair.find('=');
            if (eq != std::string::npos)
                data[urlDecode(pair.substr(0, eq))] = urlDecode(pair.substr(eq + 1));
        }
        return data;
    }
};

std::string trim(const std::string& str);
