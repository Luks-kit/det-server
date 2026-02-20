#pragma once
#include <memory>
#include <vector>
#include "script_lexer.hpp"
#include "value.hpp" // Your Value class with operator overloads

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual Value reduce(struct ScriptContext& ctx) = 0;
};
class LiteralExpr : public ASTNode {
    Value val;
public:
    LiteralExpr(Value v) : val(std::move(v)) {}
    Value reduce(ScriptContext& ctx) override { return val; }
};

class VariableExpr : public ASTNode {
    std::string varName;
public:
    VariableExpr(std::string name) : varName(std::move(name)) {}
    Value reduce(ScriptContext& ctx) override; // Logic in .cpp
};

class BinaryExpr : public ASTNode {
    std::unique_ptr<ASTNode> left, right;
    TokenType op;
public:
    BinaryExpr(std::unique_ptr<ASTNode> l, TokenType o, std::unique_ptr<ASTNode> r)
        : left(std::move(l)), op(o), right(std::move(r)) {}
    Value reduce(ScriptContext& ctx) override;
};

// --- STATEMENTS (Perform an Action) ---

class BlockStmt : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    Value reduce(ScriptContext& ctx) override;
};

class AssignmentStmt : public ASTNode {
    std::string varName;
    std::unique_ptr<ASTNode> expression;
public:
    AssignmentStmt(std::string name, std::unique_ptr<ASTNode> expr)
        : varName(std::move(name)), expression(std::move(expr)) {}
    Value reduce(ScriptContext& ctx) override;
};

class IfStmt : public ASTNode {
    std::unique_ptr<ASTNode> condition, thenBranch, elseBranch;
public:
    IfStmt(std::unique_ptr<ASTNode> c, std::unique_ptr<ASTNode> t, std::unique_ptr<ASTNode> e)
        : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
    Value reduce(ScriptContext& ctx) override;
};

class CommandStmt : public ASTNode {
    std::string command;
    std::vector<std::unique_ptr<ASTNode>> arguments;
public:
    CommandStmt(std::string cmd, std::vector<std::unique_ptr<ASTNode>> args)
        : command(std::move(cmd)), arguments(std::move(args)) {}
    Value reduce(ScriptContext& ctx) override;
};

class ForStmt : public ASTNode {
    std::string itemVar;
    std::string listName;
    std::unique_ptr<ASTNode> body;
public:
    ForStmt(std::string var, std::string list, std::unique_ptr<ASTNode> b)
        : itemVar(std::move(var)), listName(std::move(list)), body(std::move(b)) {}
    Value reduce(ScriptContext& ctx) override;
};

// For lists: [1, 2, 3]
class ListLiteralExpr : public ASTNode {
public:
    std::vector<std::unique_ptr<ASTNode>> elements;
    Value reduce(ScriptContext& ctx) override;
};

// For objects: {name: "Moses", age: 30}
class ObjectLiteralExpr : public ASTNode {
public:
    // Key is a string, value is an ASTNode (could be a literal, variable, or another list/object)
    std::map<std::string, std::unique_ptr<ASTNode>> pairs;
    Value reduce(ScriptContext& ctx) override;
};