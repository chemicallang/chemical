using namespace std;
using namespace md;

func no_highlight(lang : std::string_view, code : std::string_view) : std::string {
    return std::string()
}

func no_rewrite(url : std::string_view) : std::string {
    return std::string(url.data(), url.size())
}

func md_view_equals(env : &mut TestEnv, got : &std::string_view, expected : &std::string_view) {
    if(got.equals(expected)) {
        return;
    }
    env.error("md render mismatch");
    var expected_s = std::string("expected:\"");
    expected_s.append_view(expected)
    expected_s.append('"');
    env.info(expected_s.data())

    var got_s = std::string("got     :\"");
    got_s.append_view(got)
    got_s.append('"');
    env.info(got_s.data())
}

func md_render_equals(env : &mut TestEnv, input : &std::string_view, expected : &std::string_view) {
    var view = std::string_view(input.data(), input.size())
    var html = md::to_html(view, no_highlight, no_rewrite)
    md_view_equals(env, html.to_view(), expected)
}

@test
func test_md_header_renders(env : &mut TestEnv) {
    var input = std::string_view("# Hello")
    var expected = std::string_view("<h1 class=\"md-hg md-h1\">Hello</h1>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_header_level_three(env : &mut TestEnv) {
    var input = std::string_view("### Sub")
    var expected = std::string_view("<h3 class=\"md-hg md-h3\">Sub</h3>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_bold_and_italic(env : &mut TestEnv) {
    var input = std::string_view("**bold** and *italic*")
    var expected = std::string_view("<p class=\"md-p\"><strong class=\"md-bold\">bold</strong> and <em class=\"md-italic\">italic</em></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_fenced_code_block(env : &mut TestEnv) {
    var input = std::string_view("```\nint x = 1;\n```")
    var expected = std::string_view("<pre class=\"md-pre\"><code class=\"md-code-block\">int x = 1;\n</code></pre>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_fenced_code_with_language(env : &mut TestEnv) {
    var input = std::string_view("```cpp\nint x;\n```")
    var expected = std::string_view("<pre class=\"md-pre\"><code class=\"md-code-block language-cpp\">int x;\n</code></pre>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_link_renders(env : &mut TestEnv) {
    var input = std::string_view("[chemical](https://chemical-lang.org)")
    var expected = std::string_view("<p class=\"md-p\"><a class=\"md-link\" href=\"https://chemical-lang.org\">chemical</a></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_image_renders(env : &mut TestEnv) {
    var input = std::string_view("![alt text](img.png)")
    var expected = std::string_view("<p class=\"md-p\"><img class=\"md-img\" src=\"img.png\" alt=\"alt text\"/></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_unordered_list(env : &mut TestEnv) {
    var input = std::string_view("- one\n- two")
    var expected = std::string_view("<ul class=\"md-ul\"><li class=\"md-li\"> one</li>\n<li class=\"md-li\"> two</li>\n</ul>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_ordered_list(env : &mut TestEnv) {
    var input = std::string_view("1. one\n2. two")
    var expected = std::string_view("<ol class=\"md-ol\"><li class=\"md-li\"> one</li>\n<li class=\"md-li\"> two</li>\n</ol>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_nested_list(env : &mut TestEnv) {
    var input = std::string_view("- parent\n  - child\n- sibling")
    var expected = std::string_view("<ul class=\"md-ul\"><li class=\"md-li\"> parent<ul class=\"md-ul\"><li class=\"md-li\"> child</li>\n</ul>\n</li>\n<li class=\"md-li\"> sibling</li>\n</ul>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_task_checkbox(env : &mut TestEnv) {
    var input = std::string_view("- [x] done\n- [ ] todo")
    var expected = std::string_view("<ul class=\"md-ul\"><li class=\"md-li\"><input class=\"md-task-checkbox\" type=\"checkbox\" disabled checked/> done</li>\n<li class=\"md-li\"><input class=\"md-task-checkbox\" type=\"checkbox\" disabled/> todo</li>\n</ul>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_blockquote(env : &mut TestEnv) {
    var input = std::string_view("> quote")
    var expected = std::string_view("<blockquote class=\"md-blockquote\"> quote</blockquote>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_inline_code(env : &mut TestEnv) {
    var input = std::string_view("use `md::to_html`")
    var expected = std::string_view("<p class=\"md-p\">use <code class=\"md-code\">md::to_html</code></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_table_renders(env : &mut TestEnv) {
    var input = std::string_view("| A | B |\n|---|---|\n| 1 | 2 |")
    var expected = std::string_view("<table class=\"md-table\">\n<thead class=\"md-thead\">\n<tr class=\"md-tr\"><th class=\"md-th\"> A </th><th class=\"md-th\"> B </th></tr>\n</thead>\n<tbody class=\"md-tbody\">\n<tr class=\"md-tr\"><td class=\"md-td\"> 1 </td><td class=\"md-td\">2 </td></tr>\n</tbody>\n</table>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_strikethrough(env : &mut TestEnv) {
    var input = std::string_view("~~gone~~")
    var expected = std::string_view("<p class=\"md-p\"><del class=\"md-del\">gone</del></p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_paragraph_break(env : &mut TestEnv) {
    var input = std::string_view("line one\n\nline two")
    var expected = std::string_view("<p class=\"md-p\">line one</p>\n<p class=\"md-p\">line two</p>\n")
    md_render_equals(env, &input, &expected)
}

@test
func test_md_highlighter_is_used(env : &mut TestEnv) {
    var input = std::string_view("```cpp\nint x;\n```")
    var highlight : (lang : std::string_view, code : std::string_view) => std::string = (lang, code) => {
        if(lang.equals("cpp")) {
            var res = std::string("<span class=\"hl\">");
            res.append_view(&code);
            res.append_view("</span>");
            return res
        }
        return std::string()
    }
    var rewrite : (url : std::string_view) => std::string = (url) => {
        return std::string(url.data(), url.size())
    }
    var html = md::to_html(input, highlight, rewrite)
    var expected = std::string_view("<pre class=\"md-pre\"><code class=\"md-code-block language-cpp\"><span class=\"hl\">int x;\n</span></code></pre>\n")
    md_view_equals(env, html.to_view(), &expected)
}

@test
func test_md_link_rewriter_is_used(env : &mut TestEnv) {
    var input = std::string_view("[x](old-url)")
    var highlight : (lang : std::string_view, code : std::string_view) => std::string = (lang, code) => {
        return std::string()
    }
    var rewrite : (url : std::string_view) => std::string = (url) => {
        var res = std::string("/prefix/");
        res.append_view(&url);
        return res
    }
    var html = md::to_html(input, highlight, rewrite)
    var expected = std::string_view("<p class=\"md-p\"><a class=\"md-link\" href=\"/prefix/old-url\">x</a></p>\n")
    md_view_equals(env, html.to_view(), &expected)
}
