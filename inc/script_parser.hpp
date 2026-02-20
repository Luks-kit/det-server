#pragma once
#include "script_lexer.hpp"
#include "script_ast.hpp"
#include <functional>
#include <map>

enum Precedence {
    NONE,
    ASSIGNMENT, // =
    EQUALITY,   // == !=
    COMPARISON, // < >
    TERM,       // + -
    FACTOR,     // * /
    PRIMARY
};

class ScriptParser {
    std::vector<Token> tokens;
    size_t current = 0;

public:
    ScriptParser(std::vector<Token> t) : tokens(std::move(t)) {}
    std::unique_ptr<ASTNode> parseProgram();

private:
    std::unique_ptr<ASTNode> parseExpression(Precedence prec);
    std::unique_ptr<ASTNode> parseStatement();
    std::unique_ptr<ASTNode> parseList();
    std::unique_ptr<ASTNode> parseObject();
    
    // Pratt helpers
    Token advance() { return tokens[current++]; }
    Token peek() { return tokens[current]; }
    void consume(TokenType type, std::string msg);
    bool match(TokenType type);
    
    // Binding Power lookup
    Precedence getPrecedence(TokenType type);
};