
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

    func toStringHtmlOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageHtml.size())
        str.append_with_len(pageHtml.data(), pageHtml.size())
        return str;
    }

    func htmlToString(&self, linkedStylesheet : &std::string_view) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageHtml.size() + 80)
        var start = std::string_view("<!DOCTYPE html><html><head>")
        str.append_with_len(start.data(), start.size())
        str.append_with_len(pageHead.data(), pageHead.size())
        if(!linkedStylesheet.empty()) {
            const linkStart = std::string_view("<link rel=\"stylesheet\" href=\"");
            str.append_with_len(linkStart.data(), linkStart.size());
            str.append_with_len(linkedStylesheet.data(), linkedStylesheet.size());
            const linkEnd = std::string_view("\">")
            str.append_with_len(linkEnd.data(), linkEnd.size());
        }
        var bodyStart = std::string_view("</head><body>")
        str.append_with_len(bodyStart.data(), bodyStart.size())
        str.append_with_len(pageHtml.data(), pageHtml.size())
        var end = std::string_view("</body></html>")
        str.append_with_len(end.data(), end.size())
        return str;
    }

    func writeToFile(&self, path : &std::string_view) {
        // TODO use stream
        var completePage = toString();
        fs::write_to_file(path.data(), completePage.data())
    }

    func writeHtmlToFile(&self, path : &std::string_view, linkedStylesheet : &std::string_view) {
        // TODO use stream
        var htmlPage = htmlToString(linkedStylesheet)
        fs::write_to_file(path.data(), htmlPage.data())
    }

    func writeCssToFile(&self, path : &std::string_view) {
        // TODO use stream
        fs::write_to_file(path.data(), pageCss.data())
    }

    func writeToDirectory(&self, path : &std::string_view, name : &std::string_view) {

        // TODO only if not exists
        fs::mkdir(path.data());

        // creating the route file at
        var htmlFile = std::string(path.data(), path.size())
        htmlFile.append('/');
        htmlFile.append_with_len(name.data(), name.size());
        htmlFile.append_char_ptr(".html")

        const cssFileName = std::string()
        if(!pageCss.empty()) {
            cssFileName.append_with_len(name.data(), name.size())
            cssFileName.append_char_ptr(".css");
        }

        // writing only html to route
        writeHtmlToFile(std::string_view(htmlFile.data(), htmlFile.size()), std::string_view(cssFileName.data(), cssFileName.size()))

        if(pageCss.empty()) {
            return;
        }

        const cssFile = std::string(path.data(), path.size())
        cssFile.append('/');
        cssFile.append_with_len(cssFileName.data(), cssFileName.size())

        writeCssToFile(std::string_view(cssFile.data(), cssFile.size()))

    }

}