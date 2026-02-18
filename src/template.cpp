#include "parser.hpp"
#include <sstream>
#include <algorithm>

// Utility function to clean up whitespace
std::string trim(const std::string& s) {
    size_t first = s.find_first_not_of(" \t\n\r");
    if (std::string::npos == first) return "";
    size_t last = s.find_last_not_of(" \t\n\r");
    return s.substr(first, (last - first + 1));
}

// Ensure this matches your Header declaration
class TemplateParser {
    std::string html;
    size_t pos = 0;

public:
    TemplateParser(std::string h) : html(h) {}

    std::vector<std::unique_ptr<Node>> parse(std::string stopTag = "") {
        std::vector<std::unique_ptr<Node>> nodes;
        while (pos < html.length()) {
            size_t start = html.find("{", pos);
            if (start == std::string::npos) {
                nodes.push_back(std::make_unique<TextNode>(html.substr(pos)));
                break;
            }

            nodes.push_back(std::make_unique<TextNode>(html.substr(pos, start - pos)));
            pos = start;

            if (html.substr(pos, 2) == "{{") {
                size_t end = html.find("}}", pos);
                std::string varName = trim(html.substr(pos + 2, end - pos - 2));
                nodes.push_back(std::make_unique<VarNode>(varName));
                pos = end + 2;
            } 
            else if (html.substr(pos, 2) == "{%") {
                size_t end = html.find("%}", pos);
                std::string content = trim(html.substr(pos + 2, end - pos - 2));
                
                if (!stopTag.empty() && content == stopTag) {
                    pos = end + 2;
                    return nodes; // Return control to parent block
                }

                nodes.push_back(parseTag(content));
                pos = end + 2;
            } else {
                nodes.push_back(std::make_unique<TextNode>("{"));
                pos++;
            }
        }
        return nodes;
    }

    std::unique_ptr<Node> parseTag(const std::string& content) {
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
};

// This matches the Template::render call in your router
std::string Template::render(std::string html, const RenderContext& ctx) {
    TemplateParser parser(html);
    auto nodes = parser.parse();
    std::string result;
    for (auto& node : nodes) {
        result += node->render(ctx);
    }
    return result;
}