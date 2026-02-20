#include "script_lexer.hpp"
#include <cctype>
#include <map>

std::vector<Token> ScriptLexer::tokenize() {
    std::vector<Token> tokens;

    // Map for keyword lookups
    static const std::map<std::string, TokenType> keywords = {
        {"set", TokenType::SET},
        {"if", TokenType::IF},
        {"else", TokenType::ELSE},
        {"for", TokenType::FOR},
        {"end", TokenType::END},
        {"render", TokenType::RENDER},
        {"redirect", TokenType::REDIRECT},
        {"save_session", TokenType::SAVE_SESSION},
        {"add_cookie",   TokenType::ADD_COOKIE},
        {"set_session",  TokenType::SET_SESSION}
    };

    while (!isAtEnd()) {
        char c = next();

        if (std::isspace(c)) {
            if (c == '\n') line++;
            continue;
        }

        // Comments (# style)
        if (c == '#') {
            while (peek() != '\n' && !isAtEnd()) next();
            continue;
        }

        // Numbers
        if (std::isdigit(c)) {
            std::string num(1, c);
            while (std::isdigit(peek())) num += next();
            tokens.push_back({TokenType::NUMBER, num, line});
        }
        // Identifiers and Keywords
        else if (std::isalpha(c) || c == '_') {
            std::string text(1, c);
            while (std::isalnum(peek()) || peek() == '_' || peek() == '.') {
                text += next();
            }
            
            if (keywords.count(text)) {
                tokens.push_back({keywords.at(text), text, line});
            } else {
                tokens.push_back({TokenType::IDENTIFIER, text, line});
            }
        }
        // String Literals
        else if (c == '"' || c == '\'') {
            char quote = c;
            std::string str;
            while (peek() != quote && !isAtEnd()) {
                if (peek() == '\n') line++;
                str += next();
            }
            if (!isAtEnd()) next(); // Consume closing quote
            tokens.push_back({TokenType::STRING, str, line});
        }
        // Operators (Multi-character check)
        else {
            switch (c) {
                case '=':
                    if (peek() == '=') {
                        next();
                        tokens.push_back({TokenType::EQUAL_EQUAL, "==", line});
                    } else {
                        tokens.push_back({TokenType::EQUAL, "=", line});
                    }
                    break;
                case '!':
                    if (peek() == '=') {
                        next();
                        tokens.push_back({TokenType::BANG_EQUAL, "!=", line});
                    }
                    break;
                case '<': tokens.push_back({TokenType::LESS, "<", line}); break;
                case '>': tokens.push_back({TokenType::GREATER, ">", line}); break;
                case '+': tokens.push_back({TokenType::PLUS, "+", line}); break;
                case '-': tokens.push_back({TokenType::MINUS, "-", line}); break;
                case '*': tokens.push_back({TokenType::STAR, "*", line}); break;
                case '/': tokens.push_back({TokenType::SLASH, "/", line}); break;
                case '(': tokens.push_back({TokenType::LPAREN, "(", line}); break;
                case ')': tokens.push_back({TokenType::RPAREN, ")", line}); break;
                case '[': tokens.push_back({TokenType::L_BRACKET, "{", line}); break;
                case ']': tokens.push_back({TokenType::R_BRACKET, "}", line}); break;
                case '{': tokens.push_back({TokenType::L_BRACE, "[", line}); break;
                case '}': tokens.push_back({TokenType::R_BRACE, "]", line}); break;
                case ':': tokens.push_back({TokenType::COLON, ":", line}); break;
                case ';': tokens.push_back({TokenType::SEMICOLON, ";", line}); break;
                case ',': tokens.push_back({TokenType::COMMA, ",", line}); break;
                default:
                    // Log unknown character or throw error
                    break;
            }
        }
    }

    tokens.push_back({TokenType::EOF_TOKEN, "", line});
    return tokens;
}

bool ScriptLexer::isAtEnd() {
    return cursor >= source.size();
}