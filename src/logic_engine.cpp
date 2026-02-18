#include "router.hpp"
#include <fstream>
#include <sstream>

class LogicEngine {
    struct ScriptContext {
        const HttpRequest& req;
        HttpResponse& res;
        std::map<std::string, std::string> vars;
        std::map<std::string, std::string> form;
    };

public:
    static void execute(std::string path, const HttpRequest& req, HttpResponse& res) {
        std::ifstream file(path);
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) lines.push_back(line);

        ScriptContext ctx{req, res};
        // Pre-parse form if it's a POST
        // ctx.form = Parser::parseForm(req.body); 

        process(lines, 0, lines.size(), ctx);
    }

private:
    static size_t process(const std::vector<std::string>& lines, size_t start, size_t end, ScriptContext& ctx) {
        for (size_t i = start; i < end; ++i) {
            std::stringstream ss(lines[i]);
            std::vector<std::string> tokens;
            std::string t;
            while (ss >> t) tokens.push_back(t);

            if (tokens.empty() || tokens[0][0] == '#') continue;

            if (tokens[0] == "if") {
                size_t endif = findEnd(lines, i, "endif");
                size_t else_idx = findEnd(lines, i, "else");
                
                bool cond = (ctx.vars[tokens[1]] == tokens[3]); // Simple 'var == val'
                if (cond) process(lines, i + 1, (else_idx < endif ? else_idx : endif), ctx);
                else if (else_idx < endif) process(lines, else_idx + 1, endif, ctx);
                i = endif;
            } else {
                run(tokens, ctx);
            }
        }
        return end;
    }

    static void run(const std::vector<std::string>& tokens, ScriptContext& ctx) {
        std::string cmd = tokens[0];
        if (cmd == "set") ctx.vars[tokens[1]] = tokens[3];
        else if (cmd == "redirect") {
            ctx.res.status = "302 Found";
            ctx.res.headers["Location"] = tokens[1];
        }
        else if (cmd == "add_cookie") ctx.res.add_cookie(tokens[1], ctx.vars[tokens[2]]);
        // ... more commands
    }

    static size_t findEnd(const std::vector<std::string>& lines, size_t start, std::string target) {
        int depth = 0;
        for (size_t i = start + 1; i < lines.size(); ++i) {
            if (lines[i].find("if ") != std::string::npos) depth++;
            if (lines[i].find("endif") != std::string::npos) {
                if (depth == 0 && target == "endif") return i;
                depth--;
            }
            if (lines[i].find("else") != std::string::npos && depth == 0 && target == "else") return i;
        }
        return lines.size();
    }
};