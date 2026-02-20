#include "script_ast.hpp"
#include "logic_engine.hpp" 
#include "session_store.hpp"
#include "logger.hpp"
#include <iostream>

// --- Expressions ---

Value VariableExpr::reduce(ScriptContext& ctx) {
    // 1. Handle form.variable
    if (varName.rfind("form.", 0) == 0) {
        std::string key = varName.substr(5);
        Logger::log(LogLevel::DEBUG, "[AST] Looking up form data: " + key);
        return ctx.form.count(key) ? Value(ctx.form.at(key)) : Value("");
    }

    // 2. Handle session.variable
    if (varName.rfind("session.", 0) == 0) {
        std::string key = varName.substr(8);
        std::string sid = ctx.req.cookies.count(key) ? ctx.req.cookies.at(key) : "";
        if (sid.empty()) {
            Logger::log(LogLevel::WARN, "[AST] Session lookup failed: No 'sid' cookie found");
            return Value("");
        }
        std::string sessionData = SessionStore::get(sid);
        Logger::log(LogLevel::DEBUG, "[AST] Session retrieved for SID: " + sid);
        return Value(sessionData);
    }

    // 3. Handle cookie.variable
    if (varName.rfind("cookie.", 0) == 0) {
        std::string key = varName.substr(7);
        Logger::log(LogLevel::DEBUG, "[AST] Looking up cookie: " + key);
        return ctx.req.cookies.count(key) ? Value(ctx.req.cookies.at(key)) : Value("");
    }

    // 4. Fallback to local script variables
    if (ctx.vars.count(varName)) {
        return ctx.vars.at(varName);
    }

    Logger::log(LogLevel::DEBUG, "[AST] Variable not found, returning empty: " + varName);
    return Value(""); 
}

Value BinaryExpr::reduce(ScriptContext& ctx) {
    Value leftVal = left->reduce(ctx);
    Value rightVal = right->reduce(ctx);

    Logger::log(LogLevel::DEBUG, "[AST] Binary Op: " + leftVal.asString() + " [Op] " + rightVal.asString());

    switch (op) {
        case TokenType::PLUS:          return leftVal.asInt() + rightVal.asInt();
        case TokenType::MINUS:         return leftVal.asInt() - rightVal.asInt();
        case TokenType::STAR:          return leftVal.asInt() * rightVal.asInt();
        case TokenType::SLASH: {
            if (rightVal.asInt() == 0) {
                Logger::log(LogLevel::ERR, "[AST] Runtime Error: Division by zero");
                return Value(0);
            }
            return leftVal.asInt() / rightVal.asInt();
        }
        case TokenType::EQUAL_EQUAL:   return Value(leftVal == rightVal);
        case TokenType::BANG_EQUAL:    return Value(leftVal != rightVal);
        case TokenType::LESS:          return Value(leftVal < rightVal);
        case TokenType::GREATER:       return Value(leftVal > rightVal);
        default:                       return Value();
    }
}

// --- Statements ---

Value BlockStmt::reduce(ScriptContext& ctx) {
    Value last;
    for (auto& s : statements) {
        if (s) { // The Shield: prevent segfault if parser messed up
            last = s->reduce(ctx);
        }
        // Optimization: If a redirect was triggered, stop executing the rest of the script
        if (ctx.res.status == "302 Found") return last;
    }
    return last;
}
Value AssignmentStmt::reduce(ScriptContext& ctx) {
    Value val = expression->reduce(ctx);
    Logger::log(LogLevel::INFO, "[AST] Assign: " + varName + " = " + val.asString());
    ctx.vars[varName] = val;
    return Value();
}

Value IfStmt::reduce(ScriptContext& ctx) {
    Value condResult = condition->reduce(ctx);
    
    if (condResult.isTruthy()) {
        Logger::log(LogLevel::DEBUG, "[AST] If condition TRUE, entering 'then' branch");
        return thenBranch->reduce(ctx);
    } else if (elseBranch) {
        Logger::log(LogLevel::DEBUG, "[AST] If condition FALSE, entering 'else' branch");
        return elseBranch->reduce(ctx);
    }
    return {};
}

Value CommandStmt::reduce(ScriptContext& ctx) {
    if (command == "render") {
        if (arguments.empty()) return {};
        std::string templatePath = arguments[0]->reduce(ctx).asString();
        
        Logger::log(LogLevel::INFO, "[AST] Rendering template: " + templatePath);
        
        RenderContext t_ctx;
        t_ctx.vars = ctx.vars; 

        std::string rawHtml = Router::readFile("/" + templatePath);
        ctx.res.body = Template::render(rawHtml, t_ctx);
        ctx.res.status = "200 OK";
        ctx.res.headers["Content-Type"] = "text/html";
        return {};
    } 
    else if (command == "save_session") {
        if (arguments.size() < 2) {
             Logger::log(LogLevel::ERR, "[AST] save_session: missing arguments");
             return {};
        }
        std::string sid = arguments[0]->reduce(ctx).asString();
        std::string user = arguments[1]->reduce(ctx).asString();
        
        Logger::log(LogLevel::INFO, "[AST] Saving session for user: " + user);
        SessionStore::save(sid, user);
        return {};
    }
    else if (command == "add_cookie") {
        std::string key = arguments[0]->reduce(ctx).asString(); 
        std::string val = arguments[1]->reduce(ctx).asString();
        
        Logger::log(LogLevel::INFO, "[AST] Setting cookie: " + key + "=" + val);
        ctx.res.add_cookie(key, val);
        return {};
    }
    else if (command == "redirect") {
        if (arguments.empty()) {
            Logger::log(LogLevel::ERR, "[AST] Redirect command missing URL");
            return {};
        }
        std::string url = arguments[0]->reduce(ctx).asString();
        Logger::log(LogLevel::INFO, "[AST] Redirecting to: " + url);
        ctx.res.status = "302 Found";
        ctx.res.headers["Location"] = url;
        ctx.res.body = ""; 
        return {};
    }
    return {};
}

Value ForStmt::reduce(ScriptContext& ctx) {
    if (ctx.lists.count(listName)) {
        Logger::log(LogLevel::DEBUG, "[AST] Entering loop over list: " + listName);
        Value oldVal = ctx.vars.count(itemVar) ? ctx.vars[itemVar] : Value();

        size_t count = 0;
        for (const auto& itemMap : ctx.lists.at(listName)) {
            if (itemMap.count("value")) {
                ctx.vars[itemVar] = itemMap.at("value");
                body->reduce(ctx);
                count++;
            }
        }
        Logger::log(LogLevel::DEBUG, "[AST] Loop finished. Iterations: " + std::to_string(count));
        ctx.vars[itemVar] = oldVal;
    } else {
        Logger::log(LogLevel::WARN, "[AST] For loop failed: list '" + listName + "' not found");
    }
    return Value();
}

Value ListLiteralExpr::reduce(ScriptContext& ctx) {
    std::vector<Value> listResult;
    for (auto& element : elements) {
        listResult.push_back(element->reduce(ctx));
    }
    Logger::log(LogLevel::DEBUG, "[AST] Created List Literal with " + std::to_string(listResult.size()) + " elements");
    return Value(listResult);
}

Value ObjectLiteralExpr::reduce(ScriptContext& ctx) {
    std::map<std::string, Value> objResult;
    for (auto const& [key, expr] : pairs) {
        objResult[key] = expr->reduce(ctx);
    }
    Logger::log(LogLevel::DEBUG, "[AST] Created Object Literal with " + std::to_string(objResult.size()) + " pairs");
    return Value(objResult);
}