
struct HtmlElseIf {

    var condition : *mut Value

    var body : std::vector<*mut HtmlChild>

}

struct HtmlIfStatement : HtmlChild {

    var condition : *mut Value

    var body : std::vector<*mut HtmlChild>

    var else_ifs : std::vector<*mut HtmlElseIf>

    var else_body : std::vector<*mut HtmlChild>

}
