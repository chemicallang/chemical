import "@std/string.ch"
import "@std/unordered_map.ch"
import "@std/string_view.ch"
import "@std/std.ch"

public struct HtmlPage {

    var pageHead : std::string

    var pageHtml : std::string

    var pageCss : std::string

    // we track which classes are done through this unordered map
    // TODO using ubigint, instead need to use size_t
    var doneClasses : std::unordered_map<ubigint, bool>

    func append_html(&mut self, value : *char, len : size_t) {
        pageHtml.append_with_len(value, len);
    }

    func append_html_char_ptr(&mut self, value : *char) {
        pageHtml.append_char_ptr(value);
    }

    func append_html_char(&mut self, value : char) {
        pageHtml.append(value)
    }

    func append_css(&mut self, value : *char, len : size_t, hash : size_t) {
        if(!doneClasses.contains(hash)) {
            doneClasses.insert(hash, true)
            pageCss.append_with_len(value, len);
        }
    }

    func append_css_char_ptr(&mut self, value : *char) {
        pageCss.append_char_ptr(value);
    }

    func append_css_char(&mut self, value : char) {
        pageCss.append(value)
    }

    func append_char_ptr(&mut self, value : *char) {
        pageHtml.append_char_ptr(value);
    }

    func append(&mut self, value : char) {
        pageHtml.append(value);
    }

    func toString(&self) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + 80)
        var start = std::string_view("<!DOCTYPE html><html><head>")
        str.append_with_len(start.data(), start.size())
        str.append_with_len(pageHead.data(), pageHead.size())
        const cssStart = std::string_view("<style>")
        str.append_with_len(cssStart.data(), cssStart.size())
        str.append_with_len(pageCss.data(), pageCss.size())
        const cssEnd = std::string_view("</style>")
        str.append_with_len(cssEnd.data(), cssEnd.size())
        var bodyStart = std::string_view("</head><body>")
        str.append_with_len(bodyStart.data(), bodyStart.size())
        str.append_with_len(pageHtml.data(), pageHtml.size())
        var end = std::string_view("</body></html>")
        str.append_with_len(end.data(), end.size())
        return str;
    }

}