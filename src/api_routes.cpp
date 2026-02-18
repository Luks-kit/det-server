#include "router.hpp"


HttpResponse icon_detail(const HttpRequest& req, const std::smatch& args) {
    // args[0] is the whole path, args[1] is the first ([0-9]+) group
    std::string icon_id = args[1].str(); 
    
    return HttpResponse{"Displaying icon: " + icon_id};
}
// This "Macro" acts like the @router.get decorator
REGISTER_ROUTE("GET", "/icon/([0-9]+)", icon_detail);

HttpResponse post_comment(const HttpRequest& req, const std::smatch& args) {
    return HttpResponse{"Comment posted: " + req.body};
}
REGISTER_ROUTE("POST", "/comment", post_comment);

HttpResponse homepage(const HttpRequest& req, const std::smatch& args) {
    return HttpResponse{Router::readFile("/index.html"), "text/html"};
}
REGISTER_ROUTE("GET", "/", homepage);