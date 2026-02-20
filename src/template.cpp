#include "parser.hpp"
#include "template.hpp"
#include "logger.hpp"
#include <sstream>
#include <algorithm>

std::vector<std::unique_ptr<Node>> TemplateParser::parse(std::string stopTag) {
    std::vector<std::unique_ptr<Node>> nodes;
    while (pos < input.length()) {
        size_t start = input.find("{", pos);
        if (start == std::string::npos) {
            nodes.push_back(std::make_unique<TextNode>(input.substr(pos)));
            break;
        }

        nodes.push_back(std::make_unique<TextNode>(input.substr(pos, start - pos)));
        pos = start;

        if (input.substr(pos, 2) == "{{") {
            size_t end = input.find("}}", pos);
            std::string varName = trim(input.substr(pos + 2, end - pos - 2));
            nodes.push_back(std::make_unique<VarNode>(varName));
            pos = end + 2;

        } else if (input.substr(pos, 2) == "{%") {
            size_t end = input.find("%}", pos);
            if (end == std::string::npos) {
                // Safety: no closing tag, just treat as text or throw
                nodes.push_back(std::make_unique<TextNode>("{%"));
                pos += 2;
                continue;
            }

            std::string content = trim(input.substr(pos + 2, end - pos - 2));

            // CRITICAL FIX START: 
            // Advance the position PAST the closing '%}' BEFORE recursion
            pos = end + 2; 

            if (!stopTag.empty() && content == stopTag) {
                return nodes; // We found our 'endfor' or 'endif'
            }

            // Now call parseTag. Since pos is advanced, 
            // the recursive call to parse() will start looking AFTER this tag.
            nodes.push_back(parseTag(content));
            
            // REMOVE 'pos = end + 2;' from here if it's currently after parseTag
        }else {
            nodes.push_back(std::make_unique<TextNode>("{"));
            pos++;
        }
    }
    // A simple safety check in parse()
    if (nodes.size() > 1000) { 
        Logger::log(LogLevel::ERR, "Template too complex or recursive loop detected!");
        return nodes; 
    }
}

std::unique_ptr<Node> TemplateParser::parseTag(const std::string& content) {
    if (content.find("if ") == 0) {
        auto node = std::make_unique<IfNode>();
        node->conditionVar = trim(content.substr(3));
        node->children = parse("endif");
        return node;
    } 
    else if (content.find("for ") == 0) {
        auto node = std::make_unique<ForNode>();
        std::string loopDef = trim(content.substr(4));
        size_t inPos = loopDef.find(" in ");
        if (inPos != std::string::npos) {
            node->itemVar = trim(loopDef.substr(0, inPos));
            node->listVar = trim(loopDef.substr(inPos + 4));
        }
        node->children = parse("endfor");
        return node;
    }
    return std::make_unique<TextNode>(""); // Unknown tags
}

// This matches the Template::render call in your router
std::string Template::render(std::string html, const RenderContext& ctx) {
    TemplateParser parser(html);
    auto nodes = parser.parse();
    std::string result = "";
    for (auto& node : nodes) {
        result += node->render(ctx);
    }
    return result;
}

std::string TextNode::render(const RenderContext& ctx) {
    return text;
}

std::string VarNode::render(const RenderContext& ctx) {
    return ctx.vars.count(name) ? ctx.vars.at(name) : "";
}

std::string IfNode::render(const RenderContext& ctx) {
    if (ctx.vars.count(conditionVar) && !ctx.vars.at(conditionVar).empty()) {
        std::string result = "";
        for (auto& child : children) {
            result += child->render(ctx);
        }
        return result;
    }
    return "";
}

std::string ForNode::render(const RenderContext& ctx) {
    std::string result = "";
    if (ctx.lists.count(listVar)) {
        for (const auto& item : ctx.lists.at(listVar)) {
            RenderContext loopCtx = ctx;
            
            // If the item has a "value" key (common for simple lists)
            if (item.count("value")) {
                loopCtx.vars[itemVar] = item.at("value");
            }
            // Map sub-properties: user.name, user.id
            for (const auto& [k, v] : item) {
                loopCtx.vars[itemVar + "." + k] = v;
            }
            for (auto& child : children) {
                result += child->render(loopCtx);
            }
        }
    }
    return result;
}