#pragma once
#include "router.hpp"
#include "template.hpp"
#include "parser.hpp"
#include "value.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

struct ScriptContext {
    const HttpRequest& req;
    HttpResponse& res;
    std::map<std::string, Value> vars;
    std::map<std::string, std::string> form; // For POST data
    std::map<std::string, std::vector<std::map<std::string, std::string>>> lists;
};
