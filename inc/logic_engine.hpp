#include "router.hpp"
#include <fstream>
#include <sstream>
#include <iostream>

class LogicEngine {
    struct ScriptContext {
        const HttpRequest& req;
        HttpResponse& res;
        std::map<std::string, std::string> vars;
    };

public:
    static void execute(std::string scriptPath, const HttpRequest& req, HttpResponse& res) {
        std::ifstream file(scriptPath);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) lines.push_back(line);

        ScriptContext ctx{req, res};
        processBlocks(lines, 0, lines.size(), ctx);
    }

private:
    static size_t processBlocks(const std::vector<std::string>& lines, size_t start, size_t end, ScriptContext& ctx) {
        for (size_t i = start; i < end; ++i) {
            std::string line = lines[i]; // Add trim logic here
            if (line.empty() || line[0] == '#') continue;

            std::vector<std::string> tokens;
            std::stringstream ss(line);
            std::string t;
            while (ss >> t) tokens.push_back(t);

            if (tokens[0] == "if") {
                bool cond = (ctx.vars[tokens[1]] == tokens[3]); // Simplistic == check
                size_t endif = findEndIf(lines, i);
                if (cond) processBlocks(lines, i + 1, endif, ctx);
                i = endif;
            } else {
                runCmd(tokens, ctx);
            }
        }
        return end;
    }

    static void runCmd(const std::vector<std::string>& tokens, ScriptContext& ctx) {
        std::string cmd = tokens[0];
        if (cmd == "set") ctx.vars[tokens[1]] = tokens[3];
        else if (cmd == "add_cookie") ctx.res.add_cookie(tokens[1], ctx.vars[tokens[2]]);
        else if (cmd == "redirect") {
            ctx.res.status = "302 Found";
            ctx.res.headers["Location"] = tokens[1];
        }
        else if (cmd == "render") {
            ctx.res.body = Router::readFile("/" + tokens[1]);
            // Integrate your Template::render(ctx.res.body, ctx.vars) here
        }
    }

    static size_t findEndIf(const std::vector<std::string>& lines, size_t start) {
        for (size_t i = start + 1; i < lines.size(); ++i) 
            if (lines[i].find("endif") != std::string::npos) return i;
        return lines.size();
    }
};