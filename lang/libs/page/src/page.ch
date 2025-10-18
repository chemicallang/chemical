
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

    func append_html_integer(&mut self, value : bigint) {
        pageHtml.append_integer(value)
    }

    func append_html_uinteger(&mut self, value : ubigint) {
        pageHtml.append_uinteger(value)
    }

    func append_html_float(&mut self, value : float) {
        pageHtml.append_float(value, 3)
    }

    func append_html_double(&mut self, value : double) {
        pageHtml.append_double(value, 3)
    }

    func append_css(&mut self, value : *char, len : size_t, hash : size_t) {
        if(!doneClasses.contains(hash)) {
            doneClasses.insert(hash, true)
            pageCss.append_with_len(value, len);
        }
    }

    // append css no hash
    // used by dynamic values to insert css no matter what (by randomizing class name)
    func append_css_nh(&mut self, value : *char, len : size_t) {
        pageCss.append_with_len(value, len)
    }

    func append_css_char_ptr(&mut self, value : *char) {
        pageCss.append_char_ptr(value);
    }

    func append_css_char(&mut self, value : char) {
        pageCss.append(value)
    }

    func append_css_integer(&mut self, value : bigint) {
        pageCss.append_integer(value)
    }

    func append_css_uinteger(&mut self, value : ubigint) {
        pageCss.append_uinteger(value)
    }

    func append_css_float(&mut self, value : float) {
        pageCss.append_float(value, 3)
    }

    func append_css_double(&mut self, value : double) {
        pageCss.append_double(value, 3)
    }

    func toString(&self) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + 80)
        str.append_view(std::string_view("<!DOCTYPE html><html><head>"))
        str.append_string(pageHead)
        str.append_view(std::string_view("<style>"))
        str.append_string(pageCss)
        str.append_view(std::string_view("</style>"))
        str.append_view(std::string_view("</head><body>"))
        str.append_string(pageHtml)
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    func toStringHtmlOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageHtml.size())
        str.append_string(pageHtml)
        return str;
    }

    func toStringCssOnly(&self) : std::string {
        var str = std::string()
        str.reserve(pageCss.size())
        str.append_string(pageCss)
        return str;
    }

    func appendTitle(&self, view : &std::string_view) {
        pageHead.append_view("<title>")
        pageHead.append_view(view)
        pageHead.append_view("</title>")
    }

    func appendFavicon(&self, type : &std::string_view, path : &std::string_view) {
        pageHead.append_view("<link rel=\"icon\" type=\"")
        pageHead.append_view(type)
        pageHead.append_view("\" href=\"")
        pageHead.append_view(path)
        pageHead.append_view("\">")
    }

    func appendPngFavicon(&self, path : &std::string_view) {
        appendFavicon(std::string_view("image/png"), path)
    }

    func htmlToString(&self, linkedStylesheet : &std::string_view) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageHtml.size() + 80)
        str.append_view(std::string_view("<!DOCTYPE html><html><head>"))
        str.append_string(pageHead)
        if(!linkedStylesheet.empty()) {
            str.append_view(std::string_view("<link rel=\"stylesheet\" href=\""));
            str.append_view(linkedStylesheet);
            str.append_view(std::string_view("\">"));
        }
        str.append_view(std::string_view("</head><body>"))
        str.append_string(pageHtml)
        str.append_view(std::string_view("</body></html>"))
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
        htmlFile.append_view(name);
        htmlFile.append_char_ptr(".html")

        const cssFileName = std::string()
        if(!pageCss.empty()) {
            cssFileName.append_view(name)
            cssFileName.append_char_ptr(".css");
        }

        // writing only html to route
        writeHtmlToFile(std::string_view(htmlFile.data(), htmlFile.size()), std::string_view(cssFileName.data(), cssFileName.size()))

        if(pageCss.empty()) {
            return;
        }

        const cssFile = std::string(path.data(), path.size())
        cssFile.append('/');
        cssFile.append_string(cssFileName)

        writeCssToFile(std::string_view(cssFile.data(), cssFile.size()))

    }

}