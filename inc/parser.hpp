#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>

struct RenderContext {
    std::map<std::string, std::string> vars;
    // A list is a vector of maps (each map is one 'item')
    std::map<std::string, std::vector<std::map<std::string, std::string>>> lists;
};

struct Node {
    virtual ~Node() = default;
    virtual std::string render(const RenderContext& ctx) = 0;
};

struct TextNode : Node {
    std::string text;
    TextNode(std::string t) : text(t) {}
    std::string render(const RenderContext& ctx) override { return text; }
};

struct VarNode : Node {
    std::string name;
    VarNode(std::string n) : name(n) {}
    std::string render(const RenderContext& ctx) override {
        return ctx.vars.count(name) ? ctx.vars.at(name) : "";
    }
};

struct IfNode : Node {
    std::string conditionVar;
    std::vector<std::unique_ptr<Node>> children; // Renamed to match your parser

    std::string render(const RenderContext& ctx) override {
        bool isTrue = ctx.vars.count(conditionVar) && 
                      !ctx.vars.at(conditionVar).empty() && 
                      ctx.vars.at(conditionVar) != "false" &&
                      ctx.vars.at(conditionVar) != "0";
        
        std::string result;
        if (isTrue) {
            for (auto& node : children) result += node->render(ctx);
        }
        return result;
    }
};

struct ForNode : Node {
    std::string itemVar; // Changed to match your TemplateParser
    std::string listVar; // Changed to match your TemplateParser
    std::vector<std::unique_ptr<Node>> children;

    std::string render(const RenderContext& ctx) override {
        if (!ctx.lists.count(listVar)) return "";

        std::string result;
        const auto& list = ctx.lists.at(listVar);

        for (const auto& itemDataMap : list) {
            RenderContext subCtx = ctx; 
            // itemDataMap is std::map<string, string>
            for (const auto& pair : itemDataMap) {
                // Allows access like {{ user.name }}
                subCtx.vars[itemVar + "." + pair.first] = pair.second;
                // Also allows {{ user }} if the key is "value"
                if (pair.first == "value") subCtx.vars[itemVar] = pair.second;
            }

            for (auto& node : children) {
                result += node->render(subCtx);
            }
        }
        return result;
    }
};

class Template {
public:
    static std::string render(std::string html, const RenderContext& ctx);
};