#pragma once
#include <string>
#include <map>
#include "parser.hpp"

class TemplateParser {
    std::string input;
    size_t pos = 0;

public:
    TemplateParser(std::string in) : input(in) {}

    std::vector<std::unique_ptr<Node>> parse(std::string stopTag = "");

private:
    std::unique_ptr<Node> parseTag(const std::string& content);
};