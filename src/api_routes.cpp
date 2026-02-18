#include "router.hpp"


HttpResponse icon_detail(const HttpRequest& req, const std::smatch& args) {
    // 1. Get ID from regex capture group
    std::string icon_id = args[1].str(); 

    // 2. Logic (Simulating a DB lookup)
    if (icon_id == "404") {
        return HttpResponse::html("<h1>Icon Not Found</h1>", "404 Not Found");
    }

    // 3. Load the template file (You can reuse Router::readFile)
    std::string html_template = "<h1>Icon {{id}}</h1><p>Uploaded by: {{user}}</p>";

    // 4. Render with data
    std::map<std::string, std::string> context = {
        {"id", icon_id},
        {"user", "Neo"}
    };
    
    std::string final_html = Template::render(html_template, context);

    return HttpResponse::html(final_html);
}

// @router.get("/icon/{icon_id}")
REGISTER_ROUTE("GET", "/icon/([0-9]+)", icon_detail);

HttpResponse post_comment(const HttpRequest& req, const std::smatch& args) {
    return HttpResponse{"Comment posted: " + req.body};
}
REGISTER_ROUTE("POST", "/comment", post_comment);

HttpResponse homepage(const HttpRequest& req, const std::smatch& args) {
    return HttpResponse{Router::readFile("/index.html"), "text/html"};
}
REGISTER_ROUTE("GET", "/", homepage);