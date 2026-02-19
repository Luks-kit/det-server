#include "logic_engine.hpp"
#include "logger.hpp"
#include "session_store.hpp"
#include "value.hpp"
#include <fstream>
#include <sstream>

std::map<std::string, std::string> SessionStore::sessions;


void LogicEngine::execute(std::string path, const HttpRequest& req, HttpResponse& res) {
    Logger::log(LogLevel::INFO, "Executing script: " + path);
    Logger::log(LogLevel::DEBUG, "HTTP Method: " + req.method);

    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::log(LogLevel::ERR, "Failed to open script file: " + path);
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    Logger::log(LogLevel::DEBUG, "Loaded " + std::to_string(lines.size()) + " lines from script.");

    ScriptContext ctx{req, res};

    if (req.method == "POST") {
        Logger::log(LogLevel::DEBUG, "Parsing POST form data.");
        ctx.form = Parser::parseForm(req.body);
        Logger::log(LogLevel::DEBUG, "Parsed form fields: " + std::to_string(ctx.form.size()));
    }

    Logger::log(LogLevel::INFO, "Beginning script execution.");
    processBlocks(lines, 0, lines.size(), ctx);
    Logger::log(LogLevel::INFO, "Finished script execution.");
}

size_t LogicEngine::processBlocks(
    const std::vector<std::string>& lines,
    size_t start,
    size_t end,
    ScriptContext& ctx
) {
    Logger::log(LogLevel::DEBUG,
        "Processing block from line " + std::to_string(start) +
        " to " + std::to_string(end));

    for (size_t i = start; i < end; ++i) {

        Logger::log(LogLevel::DEBUG,
            "Line " + std::to_string(i) + ": " + lines[i]);

        std::stringstream ss(lines[i]);
        std::vector<std::string> tokens;
        std::string t;

        while (ss >> t) tokens.push_back(t);

        if (tokens.empty()) {
            Logger::log(LogLevel::DEBUG, "Skipping empty line.");
            continue;
        }

        if (tokens[0][0] == '#') {
            Logger::log(LogLevel::DEBUG, "Skipping comment.");
            continue;
        }

        if (tokens[0] == "if") {

            Logger::log(LogLevel::INFO,
                "Encountered IF statement.");

            size_t endif = findEnd(lines, i, "endif");
            size_t else_idx = findEnd(lines, i, "else");

            bool cond = (ctx.vars[tokens[1]] == resolveValue(tokens[3], ctx));

            Logger::log(LogLevel::DEBUG,
                "Evaluating condition: " +
                tokens[1] + " == " + resolveValue(tokens[3], ctx) +
                " -> " + (cond ? "true" : "false"));

            if (cond) {
                Logger::log(LogLevel::INFO, "Entering IF branch.");
                processBlocks(lines, i + 1,
                    (else_idx < endif ? else_idx : endif),
                    ctx);
            }
            else if (else_idx < endif) {
                Logger::log(LogLevel::INFO, "Entering ELSE branch.");
                processBlocks(lines, else_idx + 1, endif, ctx);
            }
            else {
                Logger::log(LogLevel::DEBUG, "No ELSE branch taken.");
            }

            i = endif;
        }
        if (tokens[0] == "for") {
            
            Logger::log(LogLevel::INFO,
                "Encountered FOR loop.");

            size_t endfor = findEnd(lines, i, "endfor");
            std::string listName = tokens[3];
            std::string itemVar = tokens[1];

            Logger::log(LogLevel::DEBUG,
                "Looping over list: " + listName);

            if (ctx.lists.count(listName)) {
                for (const auto& item : ctx.lists[listName]) {
                    ctx.vars[itemVar] = item.at("value");
                    Logger::log(LogLevel::DEBUG,
                        "Loop iteration with " + itemVar + " = " + ctx.vars[itemVar]);
                    processBlocks(lines, i + 1, endfor, ctx);
                }
            }
            else {
                Logger::log(LogLevel::WARN,
                    "List '" + listName + "' not found in context.");
            }

            i = endfor;
        }
        else {
            runCmd(tokens, ctx);
        }
    }

    Logger::log(LogLevel::DEBUG,
        "Exiting block range " + std::to_string(start) +
        " to " + std::to_string(end));

    return end;
}

std::string LogicEngine::resolveValue(const std::string& token, const ScriptContext& ctx) {
    // Quoted string
    if ((token.front() == '"' && token.back() == '"') ||
        (token.front() == '\'' && token.back() == '\'')) {
        return token.substr(1, token.size() - 2);
    }
    // form.variable
    if (token.rfind("form.", 0) == 0) {
        std::string key = token.substr(5);
        return ctx.form.count(key) ? ctx.form.at(key) : "";
    }
    // session.variable
    if (token.rfind("session.", 0) == 0) {
        std::string key = token.substr(8);
        return SessionStore::get(ctx.req.cookies.count(key) ? ctx.req.cookies.at(key) : "") == key
            ? SessionStore::get(ctx.req.cookies.at(key))
            : "";
    }

    if(token.rfind("cookie.", 0) == 0) {
        std::string key = token.substr(7);
        return ctx.req.cookies.count(key) ? ctx.req.cookies.at(key) : "";
    }

    
    // existing variable
    if (ctx.vars.count(token))
        return ctx.vars.count(token) ? ctx.vars.at(token) : "";

    // fallback literal
    return token;
}


void LogicEngine::runCmd(
    const std::vector<std::string>& tokens,
    ScriptContext& ctx
) {
    std::string cmd = tokens[0];

    Logger::log(LogLevel::INFO, "Executing command: " + cmd);

    if (cmd == "set") {
        ctx.vars[tokens[1]] = resolveValue(tokens[3], ctx);
        
        Logger::log(LogLevel::DEBUG,
            "Set variable '" + tokens[1] +
            "' = '" + resolveValue(tokens[3], ctx) + "'");
    
    }

    else if (cmd == "add_cookie") {
        ctx.res.add_cookie(tokens[1], resolveValue(tokens[2], ctx));
        Logger::log(LogLevel::DEBUG,
            "Added cookie '" + tokens[1] +
            "' with value from var '" + tokens[2] + "' of '" + resolveValue(tokens[2], ctx) + "'");
    }

    else if (cmd == "redirect") {
        ctx.res.status = "302 Found";
        ctx.res.headers["Location"] = tokens[1];

        Logger::log(LogLevel::WARN,
            "Redirecting to " + tokens[1]);
    }

    else if (cmd == "render") {
        Logger::log(LogLevel::INFO,
            "Rendering template: " + tokens[1]);

        std::string rawHtml = Router::readFile("/" + tokens[1]);

        if (rawHtml.empty()) {
            Logger::log(LogLevel::ERR,
                "Template file not found: " + tokens[1]);
            return;
        }

        RenderContext t_ctx;
        t_ctx.vars = ctx.vars;

        ctx.res.body = Template::render(rawHtml, t_ctx);

        Logger::log(LogLevel::DEBUG,
            "Template rendered. Output size: " +
            std::to_string(ctx.res.body.size()) + " bytes.");
    }

    else if (cmd == "save_session") {
        std::string sessionId = resolveValue(tokens[1], ctx);
        std::string username = resolveValue(tokens[2], ctx);

        SessionStore::save(sessionId, username);

        Logger::log(LogLevel::INFO,
            "Saved session " + sessionId + " for user " + username);
    }


    else {
        Logger::log(LogLevel::WARN,
            "Unknown command: " + cmd);
    }
}


size_t LogicEngine::findEnd(
    const std::vector<std::string>& lines,
    size_t start,
    std::string target
) {
    Logger::log(LogLevel::DEBUG,
        "Searching for '" + target +
        "' starting from line " + std::to_string(start));

    int depth = 0;

    for (size_t i = start + 1; i < lines.size(); ++i) {

        if (lines[i].find("if ") != std::string::npos) {
            depth++;
        }


        if (lines[i].find("endif") != std::string::npos) {
            if (depth == 0 && target == "endif") {
                Logger::log(LogLevel::DEBUG,
                    "Found endif at line " + std::to_string(i));
                return i;
            }
            depth--;
        }

        if (lines[i].find("else") != std::string::npos &&
            depth == 0 &&
            target == "else") {

            Logger::log(LogLevel::DEBUG,
                "Found else at line " + std::to_string(i));
            return i;
        }
    }

    Logger::log(LogLevel::WARN,
        "Could not find matching '" + target + "'.");

    return lines.size();
}
