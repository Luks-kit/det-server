#pragma once
#include "router.hpp"
#include "template.hpp"
#include "parser.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

class LogicEngine {
    struct ScriptContext {
        const HttpRequest& req;
        HttpResponse& res;
        std::map<std::string, std::string> vars;
        std::map<std::string, std::string> form; // For POST data
        std::map<std::string, std::vector<std::map<std::string, std::string>>> lists;
    };

public:
    static void execute(std::string scriptPath, const HttpRequest& req, HttpResponse& res) ;

private:
    static size_t processBlocks(const std::vector<std::string>& lines, size_t start, size_t end, ScriptContext& ctx);
    static void runCmd(const std::vector<std::string>& tokens, ScriptContext& ctx);
    static std::string resolveValue(const std::string& token, const ScriptContext& ctx);
    static size_t findEnd(const std::vector<std::string>& lines, size_t start, std::string target);
};