#include <string>
#include <map>
#include "parser.hpp"

std::string Template::render(std::string html, const std::map<std::string, std::string>& data) {
    for (const auto& [key, value] : data) {
        std::string placeholder = "{{" + key + "}}";
        size_t pos = 0;
        while ((pos = html.find(placeholder, pos)) != std::string::npos) {
            html.replace(pos, placeholder.length(), value);
            pos += value.length();
        }
    }
    return html;
}
