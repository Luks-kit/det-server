#pragma once
#include <string>
#include <vector>

enum class TokenType {
    // Keywords
    SET, IF, ELSE, FOR, END, RENDER, REDIRECT, SET_SESSION,
    // Literals & Identifiers
    IDENTIFIER, STRING, NUMBER,
    // Operators
    EQUAL, EQUAL_EQUAL, BANG_EQUAL, LESS, GREATER, PLUS, MINUS, STAR, SLASH,
    // Delimiters
    LPAREN, RPAREN, EOF_TOKEN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class ScriptLexer {
    std::string source;
    size_t cursor = 0;
    int line = 1;
public:
    ScriptLexer(std::string src) : source(std::move(src)) {}
    std::vector<Token> tokenize();
private:
    char peek() { return cursor < source.size() ? source[cursor] : '\0'; }
    char next() { return source[cursor++]; }
    bool isAtEnd();
};