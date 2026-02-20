#include "script_parser.hpp"
#include <stdexcept>

Precedence ScriptParser::getPrecedence(TokenType type) {
    switch (type) {
        case TokenType::EQUAL:        return ASSIGNMENT;
        case TokenType::EQUAL_EQUAL:
        case TokenType::BANG_EQUAL:   return EQUALITY;
        case TokenType::LESS:
        case TokenType::GREATER:      return COMPARISON;
        case TokenType::PLUS:
        case TokenType::MINUS:        return TERM;
        case TokenType::STAR:
        case TokenType::SLASH:        return FACTOR;
        default:                      return NONE;
    }
}

std::unique_ptr<ASTNode> ScriptParser::parseProgram() {
    auto block = std::make_unique<BlockStmt>();
    while (peek().type != TokenType::EOF_TOKEN) {
        block->statements.push_back(parseStatement());
    }
    return block;
}

std::unique_ptr<ASTNode> ScriptParser::parseStatement() {
    if (peek().type == TokenType::SET) {
        advance(); // consume 'set'
        Token name = peek();
        consume(TokenType::IDENTIFIER, "Expect variable name");
        consume(TokenType::EQUAL, "Expect '='");
        auto expr = parseExpression(NONE);
        return std::make_unique<AssignmentStmt>(name.value, std::move(expr));
    }

    if (peek().type == TokenType::IF) {
        advance(); // consume 'if'
        auto condition = parseExpression(NONE);
        auto thenBranch = std::make_unique<BlockStmt>();
        
        while (peek().type != TokenType::ELSE && peek().type != TokenType::END) {
            thenBranch->statements.push_back(parseStatement());
        }

        std::unique_ptr<ASTNode> elseBranch = nullptr;
        if (peek().type == TokenType::ELSE) {
            advance();
            auto elseBlock = std::make_unique<BlockStmt>();
            while (peek().type != TokenType::END) {
                elseBlock->statements.push_back(parseStatement());
            }
            elseBranch = std::move(elseBlock);
        }
        consume(TokenType::END, "Expect 'end'");
        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    if (peek().type == TokenType::RENDER || peek().type == TokenType::REDIRECT || peek().type == TokenType::SET_SESSION) {

        Token cmd = advance();
        std::vector<std::unique_ptr<ASTNode>> args;

        // Simple heuristic: parse expressions until we hit a keyword or EOF
        // For a more robust version, you'd check for a NewLine token
        while (peek().type != TokenType::EOF_TOKEN && 
               peek().type != TokenType::END && 
               peek().type != TokenType::ELSE &&
               peek().type != TokenType::SET &&
               peek().type != TokenType::IF) 
        { args.push_back(parseExpression(NONE)); }

        return std::make_unique<CommandStmt>(cmd.value, std::move(args));
    }

    return parseExpression(NONE);
}

std::unique_ptr<ASTNode> ScriptParser::parseExpression(Precedence prec) {
    Token token = advance();
    std::unique_ptr<ASTNode> left;

    // Prefix (Nud)
    if (token.type == TokenType::NUMBER) {
        left = std::make_unique<LiteralExpr>(std::stoi(token.value));
    } else if (token.type == TokenType::STRING) {
        left = std::make_unique<LiteralExpr>(token.value);
    } else if (token.type == TokenType::IDENTIFIER) {
        left = std::make_unique<VariableExpr>(token.value);
    } else if (token.type == TokenType::LPAREN) {
        left = parseExpression(NONE);
        consume(TokenType::RPAREN, "Expect ')'");
    }

    // Infix (Led)
    while (prec < getPrecedence(peek().type)) {
        TokenType op = advance().type;
        auto right = parseExpression(getPrecedence(op));
        left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right));
    }

    return left;
}

void ScriptParser::consume(TokenType type, std::string msg) {
    if (peek().type == type) {
        advance();
        return;
    }
    throw std::runtime_error("Parser Error: " + msg + " on line " + std::to_string(peek().line));
}