#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "value.hpp"

struct RenderContext {
    std::map<std::string, Value> vars;
    // A list is a vector of maps (each map is one 'item')
    std::map<std::string, std::vector<std::map<std::string, std::string>>> lists;
};

class Parser {
public:
    static std::string urlDecode(const std::string& str);
    static std::map<std::string, std::string> parseForm(const std::string& body);

    
};

struct Node {
    virtual ~Node() = default;
    virtual std::string render(const RenderContext& ctx) = 0;
};

struct TextNode : Node {
    std::string text;
    TextNode(std::string t) : text(t) {}
    std::string render(const RenderContext& ctx) override;
};

struct VarNode : Node {
    std::string name;
    VarNode(std::string n) : name(n) {}
    std::string render(const RenderContext& ctx) override;
};

struct IfNode : Node {
    std::string conditionVar;
    std::vector<std::unique_ptr<Node>> children; // Renamed to match your parser

    std::string render(const RenderContext& ctx) override;
};

struct ForNode : Node {
    std::string itemVar; // Changed to match your TemplateParser
    std::string listVar; // Changed to match your TemplateParser
    std::vector<std::unique_ptr<Node>> children;

    std::string render(const RenderContext& ctx) override;
};

class Template {
public:
    static std::string render(std::string html, const RenderContext& ctx);
};

std::string trim(const std::string& s);