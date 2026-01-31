import test;
import md;
import std::io;

public func test_headers() {
    test::assert(md::to_html("# H1") == "<h1 class=\"md-hg md-h1\">H1</h1>\n");
    test::assert(md::to_html("## H2") == "<h2 class=\"md-hg md-h2\">H2</h2>\n");
}

public func test_paragraph() {
    var res = md::to_html("text");
    test::assert(res == "<p class=\"md-p\">text</p>\n");
}

public func test_bold_italic() {
    var res = md::to_html("**bold** _italic_");
    test::assert(res == "<p class=\"md-p\"><strong class=\"md-bold\">bold</strong> <em class=\"md-italic\">italic</em></p>\n");
}

public func test_list() {
    var res = md::to_html("- item");
    test::assert(res == "<ul class=\"md-ul\"><li class=\"md-li\"><p class=\"md-p\">item</p>\n</li>\n</ul>\n");
}

public func test_code_block() {
    var res = md::to_html("```c\nint main() {}\n```");
    test::assert(res == "<pre class=\"md-pre\"><code class=\"md-code-block language-c\">int main() {}\n</code></pre>\n");
}
