#pragma once
#include <string>
#include "router.hpp" // Wherever your HttpRequest/Response live
#include "script_parser.hpp"

class ScriptExecutor {
public:
    // The main entry point to run a .script file
    static void execute(const std::string& path, const HttpRequest& req, HttpResponse& res);
};