#include "script_executor.hpp"
#include "script_lexer.hpp"
#include "script_parser.hpp"
#include "logic_engine.hpp"
#include "logger.hpp"
#include <fstream>
#include <sstream>

void ScriptExecutor::execute(const std::string& path, const HttpRequest& req, HttpResponse& res) {
    Logger::log(LogLevel::INFO, "Executor: Loading " + path);

    // 1. Read the script file into a string
    std::ifstream file(path);
    if (!file.is_open()) {
        Logger::log(LogLevel::ERR, "Executor: Could not open file " + path);
        res.status = "404 Not Found";
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    // 2. Prepare the Execution Context
    // This holds variables, form data, and references to req/res
    ScriptContext ctx{req, res};
    
    // If it's a POST, we assume form-encoded data for this example
    if (req.method == "POST") {
        ctx.form = Parser::parseForm(req.body); 
    }

    try {
        // 3. Tokenize (The Lexer)
        ScriptLexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        // 4. Parse (The Pratt Parser)
        ScriptParser parser(tokens);
        std::unique_ptr<ASTNode> program = parser.parseProgram();

        // 5. Execute (The AST traversal)
        Logger::log(LogLevel::INFO, "Executor: Running AST...");
        std::map<std::string, std::string> s1 = {{"name", "Production-01"}, {"status", "Healthy"}};
        std::map<std::string, std::string> s2 = {{"name", "Staging-DB"}, {"status", "Maintenance"}};
        std::map<std::string, std::string> s3 = {{"name", "Auth-Node"}, {"status", "Healthy"}};

        ctx.lists["servers"] = {s1, s2, s3};
        program->reduce(ctx);
        Logger::log(LogLevel::INFO, "Executor: Success.");

    } catch (const std::exception& e) {
        Logger::log(LogLevel::ERR, "Script Runtime Error: " + std::string(e.what()));
        res.status = "500 Internal Server Error";
        res.body = "Script Error: " + std::string(e.what());
    }
}