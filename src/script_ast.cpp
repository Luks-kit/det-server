#include "script_ast.hpp"
#include "logic_engine.hpp" // Assuming ScriptContext is defined here
#include "session_store.hpp"
#include "logger.hpp"
#include <iostream>

// --- Expressions ---

Value VariableExpr::reduce(ScriptContext& ctx) {
    // 1. Handle form.variable
    if (varName.rfind("form.", 0) == 0) {
        std::string key = varName.substr(5);
        return ctx.form.count(key) ? Value(ctx.form.at(key)) : Value("");
    }

    // 2. Handle session.variable
    if (varName.rfind("session.", 0) == 0) {
        std::string key = varName.substr(8);
        // Look up session ID from cookies, then get data from SessionStore
        std::string sid = ctx.req.cookies.count("sid") ? ctx.req.cookies.at("sid") : "";
        return Value(SessionStore::get(sid)); 
    }

    // 3. Handle cookie.variable
    if (varName.rfind("cookie.", 0) == 0) {
        std::string key = varName.substr(7);
        return ctx.req.cookies.count(key) ? Value(ctx.req.cookies.at(key)) : Value("");
    }

    // 4. Fallback to local script variables (set x = 10)
    if (ctx.vars.count(varName)) {
        return ctx.vars.at(varName);
    }

    return Value(""); // Default empty
}

Value BinaryExpr::reduce(ScriptContext& ctx) {
    Value leftVal = left->reduce(ctx);
    Value rightVal = right->reduce(ctx);

    switch (op) {
        case TokenType::PLUS:         return leftVal + rightVal;
        case TokenType::MINUS:        return leftVal - rightVal;
        case TokenType::STAR:         return leftVal * rightVal;
        case TokenType::EQUAL_EQUAL:  return Value(leftVal == rightVal);
        case TokenType::BANG_EQUAL:   return Value(leftVal != rightVal);
        case TokenType::LESS:         return Value(leftVal < rightVal);
        case TokenType::GREATER:      return Value(leftVal > rightVal);
        default:                      return Value();
    }
}

// --- Statements ---

Value BlockStmt::reduce(ScriptContext& ctx){
    for (auto& s : statements){s->reduce(ctx);} return Value();
}

Value AssignmentStmt::reduce(ScriptContext& ctx) {
    ctx.vars[varName] = expression->reduce(ctx);
    return Value(); // Statements return "null" Value
}

Value IfStmt::reduce(ScriptContext& ctx) {
    if (condition->reduce(ctx).isTruthy()) {
        thenBranch->reduce(ctx);
    } else if (elseBranch != nullptr) {
        elseBranch->reduce(ctx);
    }
    return Value();
}


Value CommandStmt::reduce(ScriptContext& ctx) {
    if (command == "render") {
        if (arguments.empty()) return {};
        std::string templatePath = arguments[0]->reduce(ctx).asString();
        
        std::string rawHtml = Router::readFile("/" + templatePath);

        if (rawHtml.empty()) {
            ctx.res.body = "<h1>Error: Template " + templatePath + " not found</h1>";
            return {};
        }

        RenderContext t_ctx;
        for (auto const& [name, val] : ctx.vars) {
            t_ctx.vars[name] = val.asString();
        }
        t_ctx.lists = ctx.lists;
        ctx.res.body = Template::render(rawHtml, t_ctx);
        ctx.res.status = "200 OK";
        ctx.res.headers["Content-Type"] = "text/html";
    } 
    else if (command == "save_session") {
        if (arguments.size() < 2) throw std::runtime_error("save_session requires 2 args");
        
        std::string sid = arguments[0]->reduce(ctx).asString();
        std::string user = arguments[1]->reduce(ctx).asString();
        
        SessionStore::save(sid, user);
    }
    else if (command == "add_cookie") {
        if (arguments.size() < 2) throw std::runtime_error("add_cookie requires 2 args");
        
        std::string key = arguments[0]->reduce(ctx).asString();
        std::string val = arguments[1]->reduce(ctx).asString();
        
        ctx.res.add_cookie(key, val);
    }
    else if (command == "redirect") {
        if (arguments.empty()) {
            Logger::log(LogLevel::ERR, "Redirect command missing URL argument");
            return Value();
        }

        // Evaluate the first argument to get the destination URL
        std::string url = arguments[0]->reduce(ctx).asString();
        ctx.res.status = "302 Found";
        ctx.res.headers["Location"] = url;
        ctx.res.body = ""; 

        Logger::log(LogLevel::INFO, "[AST] Redirecting to: " + url);
    }

    return {};
}


Value ForStmt::reduce(ScriptContext& ctx) {
    if (ctx.lists.count(listName)) {
        // Save old value to preserve scope if variable exists
        Value oldVal = ctx.vars.count(itemVar) ? ctx.vars[itemVar] : Value();

        for (const auto& itemMap : ctx.lists.at(listName)) {
            // We assume the list contains maps; we grab the "value" key
            if (itemMap.count("value")) {
                ctx.vars[itemVar] = itemMap.at("value");
                body->reduce(ctx);
            }
        }

        // Restore scope
        ctx.vars[itemVar] = oldVal;
    }
    return Value();
}