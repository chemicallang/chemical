
public struct HtmlPage {

    var pageHead : std::string

    var pageHtml : std::string

    var pageCss : std::string

    var pageJs : std::string

    var pageHeadJs : std::string

    // we track which classes are done through this unordered map
    // TODO using ubigint, instead need to use size_t
    var doneClasses : std::unordered_map<ubigint, bool>

    // track random CSS classes (for dynamic values) to prevent duplicates
    var doneRandomClasses : std::unordered_map<ubigint, bool>

    var doneComponents : std::unordered_map<ubigint, bool>

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

    func append_head(&mut self, value : *char, len : size_t) {
        pageHead.append_with_len(value, len);
    }

    func append_head_char_ptr(&mut self, value : *char) {
        pageHead.append_char_ptr(value);
    }

    func append_head_char(&mut self, value : char) {
        pageHead.append(value)
    }

    func append_head_integer(&mut self, value : bigint) {
        pageHead.append_integer(value)
    }

    func append_head_uinteger(&mut self, value : ubigint) {
        pageHead.append_uinteger(value)
    }

    func append_head_float(&mut self, value : float) {
        pageHead.append_float(value, 3)
    }

    func append_head_double(&mut self, value : double) {
        pageHead.append_double(value, 3)
    }

    func append_css(&mut self, value : *char, len : size_t) {
        pageCss.append_with_len(value, len);
    }

    func require_css_hash(&self, hash : size_t) : bool {
        return !doneClasses.contains(hash)
    }

    func set_css_hash(&mut self, hash : size_t) {
        doneClasses.insert(hash, true)
    }

    func require_component(&self, hash : size_t) : bool {
        return !doneComponents.contains(hash)
    }

    func set_component_hash(&mut self, hash : size_t) {
        doneComponents.insert(hash, true)
    }

    func require_random_css_hash(&self, hash : size_t) : bool {
        return !doneRandomClasses.contains(hash)
    }

    func set_random_css_hash(&mut self, hash : size_t) {
        doneRandomClasses.insert(hash, true)
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

    func append_js(&mut self, value : *char, len : size_t) {
        pageJs.append_with_len(value, len);
    }

    func append_js_char_ptr(&mut self, value : *char) {
        pageJs.append_char_ptr(value);
    }

    func append_js_char(&mut self, value : char) {
        pageJs.append(value)
    }

    func append_js_integer(&mut self, value : bigint) {
        pageJs.append_integer(value)
    }

    func append_js_uinteger(&mut self, value : ubigint) {
        pageJs.append_uinteger(value)
    }

    func append_js_float(&mut self, value : float) {
        pageJs.append_float(value, 3)
    }

    func append_js_double(&mut self, value : double) {
        pageJs.append_double(value, 3)
    }

    func append_head_js(&mut self, value : *char, len : size_t) {
        pageHeadJs.append_with_len(value, len);
    }

    func append_head_js_char_ptr(&mut self, value : *char) {
        pageHeadJs.append_char_ptr(value);
    }

    func append_head_js_char(&mut self, value : char) {
        pageHeadJs.append(value)
    }

    func append_head_js_integer(&mut self, value : bigint) {
        pageHeadJs.append_integer(value)
    }

    func append_head_js_uinteger(&mut self, value : ubigint) {
        pageHeadJs.append_uinteger(value)
    }

    func append_head_js_float(&mut self, value : float) {
        pageHeadJs.append_float(value, 3)
    }

    func append_head_js_double(&mut self, value : double) {
        pageHeadJs.append_double(value, 3)
    }

    func toString(&self) : std::string {
        var str = std::string()
        str.reserve(pageHead.size() + pageCss.size() + pageHtml.size() + pageHeadJs.size() + pageJs.size() + 100)
        str.append_view(std::string_view("<!DOCTYPE html><html><head>"))
        str.append_string(pageHead)
        if(!pageCss.empty()) {
            str.append_view(std::string_view("<style>"))
            str.append_string(pageCss)
            str.append_view(std::string_view("</style>"))
        }
        if(!pageHeadJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(pageHeadJs)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</head><body>"))
        str.append_string(pageHtml)
        if(!pageJs.empty()) {
            str.append_view(std::string_view("<script>"))
            str.append_string(pageJs)
            str.append_view(std::string_view("</script>"))
        }
        str.append_view(std::string_view("</body></html>"))
        return str;
    }

    func toStringHeadOnly(&self) : std::string {
        return pageHead.copy();
    }

    func toStringHeadJsOnly(&self) : std::string {
        return pageHeadJs.copy();
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

    func toStringJsOnly(&self) : std::string {
        return pageJs.copy()
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