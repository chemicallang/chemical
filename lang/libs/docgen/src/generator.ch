public namespace docgen {

public struct HtmlGenerator {
    var config : *DocConfig
    var summary : *Summary
    var search_index : std::string
    var generated_urls : std::vector<std::string>
    var pages : std::vector<*mut SummaryItem>
}

func highlight_wrapper(lang : std::string_view, code : std::string_view) : std::string {
    if(lang.equals("chemical") || lang.equals("ch")) return highlight_chemical(code);
    if(lang.equals("chemical.mod") || lang.equals("chmod")) return highlight_chmod(code);
    if(lang.equals("c")) return highlight_c(code);
    if(lang.equals("cpp") || lang.equals("c++")) return highlight_cpp(code);
    if(lang.equals("js") || lang.equals("javascript")) return highlight_js(code);
    if(lang.equals("bash") || lang.equals("sh")) return highlight_bash(code);
    if(lang.equals("html")) return highlight_html(code);
    if(lang.equals("css")) return highlight_css(code);
    return std::string();
}

func replace_extension(path : &std::string, old_ext : std::string_view, new_ext : std::string_view) : std::string {
    if(path.ends_with(&old_ext)) {
        var str = std::string();
        str.append_view(std::string_view(path.data(), path.size() - old_ext.size()))
        str.append_view(&new_ext);
        return str;
    }
    return path.copy();
}

func get_relative_path_to_root(depth : int) : std::string {
    var s = std::string();
    var i = 0;
    while(i < depth) {
        s.append_view("../");
        i++;
    }
    if(s.size() == 0) return std::string("./");
    return s;
}

// Helper to strip HTML tags for search index
func strip_tags(html : std::string_view) : std::string {
    var res = std::string();
    var in_tag = false;
    var i = 0u;
    while(i < html.size()) {
        var c = html.data()[i];
        if(c == '<') {
            in_tag = true;
        } else if(c == '>') {
            in_tag = false;
        } else if(!in_tag) {
            res.append(c);
        }
        i++;
    }
    return res;
}

// Helper to escape JSON string
func escape_json_string(str : std::string_view) : std::string {
    var res = std::string();
    var i = 0u;
    while(i < str.size()) {
        var c = str.data()[i];
        if(c == '"') {
            res.append_view("\\\"");
        } else if(c == '\\') {
             res.append_view("\\\\");
        } else if(c == '\n') {
             res.append(' ');
        } else if(c == '\r') {
             // skip
        } else {
            res.append(c);
        }
        i++;
    }
    return res;
}

// Recursively render sidebar
func render_sidebar_item(item : *SummaryItem, current_path : std::string_view, depth : int) : std::string {
    var html = std::string("<li class=\"sidebar-item\">");
    
    if(item.link.size() > 0) {
        // Fix link extension .md -> .html (use copy to preserve original)
        var link_copy = item.link.copy();
        var link = replace_extension(&link_copy, ".md", ".html");
        html.append_view("<a href=\"");
        // Prefix with relative path to root based on current page depth
        var rel_path = get_relative_path_to_root(depth)
        html.append_view(rel_path.to_view());
        html.append_view(link.to_view());
        
        html.append_view("\"");
        if(item.link.equals_view(&current_path)) {
            html.append_view(" class=\"active\"");
        }
        html.append_view(">");
        html.append_view(item.title.to_view());
        html.append_view("</a>");
    } else {
        html.append_view("<span class=\"sidebar-header\">");
        html.append_view(item.title.to_view());
        html.append_view("</span>");
    }
    
    if(item.children.size() > 0) {
        html.append_view("<ul>");
        var i = 0u;
        while(i < item.children.size()) {
            var sb_item = render_sidebar_item(item.children.get(i), current_path, depth)
            html.append_view(sb_item.to_view());
            i++;
        }
        html.append_view("</ul>");
    }
    
    html.append_view("</li>");
    return html;
}

func str_vec_contains(vec : &std::vector<std::string>, val : std::string_view) : bool {
    var i = 0u;
    while(i < vec.size()) {
        if(vec.get_ptr(i).equals_view(&val)) return true;
        i++;
    }
    return false;
}

func is_server_side_supported(lang : std::string_view) : bool {
    if(lang.equals("chemical") || lang.equals("ch")) return true;
    if(lang.equals("chemical.mod") || lang.equals("chmod")) return true;
    if(lang.equals("c")) return true;
    if(lang.equals("cpp") || lang.equals("c++")) return true;
    if(lang.equals("js") || lang.equals("javascript")) return true;
    if(lang.equals("bash") || lang.equals("sh")) return true;
    if(lang.equals("html")) return true;
    if(lang.equals("css")) return true;
    return false;
}

func get_prism_includes(config : *DocConfig) : std::string {
    var html = std::string();
    var needs_external = false;
    
    var i = 0u;
    while(i < config.syntax_highlights.size()) {
        const hl = config.syntax_highlights.get_ptr(i);
        if(!is_server_side_supported(hl.to_view())) {
            needs_external = true;
            break;
        }
        i++;
    }
    
    if(needs_external) {
         // Core + Theme
         html.append_view("<link href=\"https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/themes/prism-tomorrow.min.css\" rel=\"stylesheet\" />\n");
         html.append_view("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/components/prism-core.min.js\"></script>\n");
         html.append_view("<script src=\"https://cdnjs.cloudflare.com/ajax/libs/prism/1.29.0/plugins/autoloader/prism-autoloader.min.js\"></script>\n");
    }
    
    return html;
}

// Build an on-this-page list from h2/h3 headings in rendered content.
// The md lib emits <h2 class="md-hg md-h2">Title\n</h2> without ids, so we
// emit links that match by heading text using text-fragment-free anchors:
// we assign ids client-side via JS (see get_default_js) keyed by heading text.
func build_toc(content : std::string_view, depth : int) : std::string {
    var html = std::string();
    var i = 0u;
    var count = 0u;
    var h2_needle = std::string_view("<h2");
    var h3_needle = std::string_view("<h3");
    var gt_needle = std::string_view(">");
    var end_needle = std::string_view("</");
    var needle_view = h2_needle;
    while(i < content.size()) {
        // find next <h2 or <h3
        var rest = content.skip(i);
        var h2r = rest.find(&h2_needle);
        var h3r = rest.find(&h3_needle);
        var h2 = (if(h2r == std::NPOS) std::NPOS else h2r + i);
        var h3 = (if(h3r == std::NPOS) std::NPOS else h3r + i);
        var next = h2;
        var is_h3 = false;
        var h3_exists = (h3 != std::NPOS);
        var h2_exists = (h2 != std::NPOS);
        if((!h2_exists && h3_exists) || (h2_exists && h3_exists && h3 < h2)) {
            next = h3;
            is_h3 = true;
            needle_view = h3_needle;
        } else {
            needle_view = h2_needle;
        }
        if(next == std::NPOS) break;
        var from_next = content.skip(next);
        var gt_r = from_next.find(&gt_needle);
        if(gt_r == std::NPOS) break;
        var close_tag = gt_r + next;
        var end_r = content.skip(close_tag).find(&end_needle);
        if(end_r == std::NPOS) break;
        var text_end = end_r + close_tag;
        // heading text region (trim whitespace)
        var start = close_tag + 1u;
        while(start < text_end && (content.get(start) == ' ' || content.get(start) == '\n' || content.get(start) == '\r' || content.get(start) == '\t')) start++;
        var end = text_end;
        while(end > start) {
            var c = content.get(end - 1u);
            if(c == ' ' || c == '\n' || c == '\r' || c == '\t') end--;
            else break;
        }
        if(end > start) {
            var htext = content.subview(start, end);
            // slug for anchor (must match the JS side slugger)
            var slug = std::string();
            var s2 = 0u;
            var last_dash = false;
            while(s2 < htext.size()) {
                var ch = htext.get(s2);
                if((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) {
                    slug.append(ch);
                    last_dash = false;
                } else if(ch >= 'A' && ch <= 'Z') {
                    slug.append((ch - 'A' + 'a'));
                    last_dash = false;
                } else if(!last_dash && slug.size() > 0) {
                    slug.append('-');
                    last_dash = true;
                }
                s2++;
            }
            html.append_view("<li class=\"");
            if(is_h3) { html.append_view("toc-l3"); } else { html.append_view("toc-l2"); }
            html.append_view("\"><a href=\"#");
            html.append_string(slug.copy());
            html.append_view("\">");
            html.append_view(&htext);
            html.append_view("</a></li>");
            count++;
        }
        i = (next + needle_view.size()) as uint;
    }
    if(count == 0) return std::string();
    var out = std::string("<ul class=\"toc-list\">");
    out.append_string(html.copy());
    out.append_view("</ul>");
    return out;
}

// Prev/Next pager from the flat reading order
func pager_html(gen : &mut HtmlGenerator, page_item : *SummaryItem, depth : int) : std::string {
    var pos = page_position(&gen.pages, page_item.link.to_view());
    if(pos == -1) return std::string();
    var rel = get_relative_path_to_root(depth);
    var html = std::string("<nav class=\"pager\">");
    if(pos > 0) {
        var prev_idx = pos - 1;
        var prev = gen.pages.get(prev_idx as uint);
        var pl = prev.link.copy();
        var plink = replace_extension(&pl, ".md", ".html");
        html.append_view("<a class=\"pager-prev\" href=\"");
        html.append_view(rel.to_view());
        html.append_view(plink.to_view());
        html.append_view("\"><span class=\"pager-dir\">&larr; Previous</span><span class=\"pager-title\">");
        html.append_view(prev.title.to_view());
        html.append_view("</span></a>");
    } else {
        html.append_view("<span class=\"pager-spacer\"></span>");
    }
    var page_total : int = gen.pages.size() as int;
    if((pos + 1) < page_total) {
        var next_idx = pos + 1;
        var next = gen.pages.get(next_idx as uint);
        var nl = next.link.copy();
        var nlink = replace_extension(&nl, ".md", ".html");
        html.append_view("<a class=\"pager-next\" href=\"");
        html.append_view(rel.to_view());
        html.append_view(nlink.to_view());
        html.append_view("\"><span class=\"pager-dir\">Next &rarr;</span><span class=\"pager-title\">");
        html.append_view(next.title.to_view());
        html.append_view("</span></a>");
    } else {
        html.append_view("<span class=\"pager-spacer\"></span>");
    }
    html.append_view("</nav>");
    return html;
}

// Flatten the summary tree into an ordered list of pages (for prev/next)
func collect_pages(items : &std::vector<*mut SummaryItem>, out : &mut std::vector<*mut SummaryItem>) {
    var i = 0u;
    while(i < items.size()) {
        var item = items.get(i);
        if(item.link.size() > 0) {
            out.push_back(item);
        }
        if(item.children.size() > 0) {
            collect_pages(&item.children, out);
        }
        i++;
    }
}

// Find a page's position in the flat reading order; -1 if not a page
func page_position(pages : &std::vector<*mut SummaryItem>, md_path : std::string_view) : int {
    var total : int = pages.size() as int;
    var i = 0;
    while(i < total) {
        if(pages.get(i as uint).link.equals_view(&md_path)) return i;
        i++;
    }
    return -1;
}

// Breadcrumb trail (Home / Section / Page) for the current page
func breadcrumb_html(item : *SummaryItem, pages_root : &std::vector<*mut SummaryItem>, depth : int) : std::string {
    var rel = get_relative_path_to_root(depth);
    var html = std::string("<div class=\"breadcrumb\"><a href=\"");
    html.append_view(rel.to_view());
    html.append_view("index.html\">Home</a>");
    // find the top-level section this page belongs to
    var i = 0u;
    while(i < pages_root.size()) {
        var top = pages_root.get(i);
        // direct page match
        var top_link_v = top.link.to_view();
        if(top.link.size() > 0 && top_link_v.equals(item.link.to_view())) {
            html.append_view("<span class=\"crumb-sep\">/</span><span class=\"crumb-here\">");
            html.append_view(top.title.to_view());
            html.append_view("</span>");
            break;
        }
        // search children
        var found = false;
        var j = 0u;
        while(j < top.children.size()) {
            var child = top.children.get(j);
            var child_link_v = child.link.to_view();
            if(child.link.size() > 0 && child_link_v.equals(item.link.to_view())) {
                html.append_view("<span class=\"crumb-sep\">/</span><span class=\"crumb-here\">");
                html.append_view(top.title.to_view());
                html.append_view("</span>");
                found = true;
                break;
            }
            // nested one more level
            var k = 0u;
            while(k < child.children.size()) {
                var gc = child.children.get(k);
                var gc_link_v = gc.link.to_view();
                if(gc.link.size() > 0 && gc_link_v.equals(item.link.to_view())) {
                    html.append_view("<span class=\"crumb-sep\">/</span><span class=\"crumb-here\">");
                    html.append_view(top.title.to_view());
                    html.append_view("</span><span class=\"crumb-sep\">/</span><span class=\"crumb-here\">");
                    html.append_view(child.title.to_view());
                    html.append_view("</span>");
                    found = true;
                    break;
                }
                k++;
            }
            if(found) break;
            j++;
        }
        if(found) break;
        i++;
    }
    html.append_view("</div>");
    return html;
}

func (gen : &mut HtmlGenerator) generate_page(title : std::string_view, content : std::string_view, output_path : std::string, relative_depth : int, current_md_path : std::string_view, page_item : *SummaryItem) {
    var html = std::string("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>""");
    html.append_view(&title);
    html.append_view(" - ");
    html.append_view(gen.config.site_name.to_view());
    html.append_view("""</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">""");
    
    // Favicon
    if(gen.config.favicon_path.size() > 0) {
        var favicon_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(gen.config.favicon_path.data(), &raw mut favicon_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("\n\t<link rel=\"icon\" type=\"image/png\" href=\"");
            var rel_path = get_relative_path_to_root(relative_depth)
            html.append_view(rel_path.to_view());
            html.append_view(std::string_view(&raw favicon_name_buf[0], len));
            html.append_view("\">");
        }
    }
    
    // SEO Meta Tags
    if(gen.config.description.size() > 0) {
        html.append_view("\n\t<meta name=\"description\" content=\"");
        html.append_view(gen.config.description.to_view());
        html.append_view("\">");
    }
    if(gen.config.author.size() > 0) {
        html.append_view("\n\t<meta name=\"author\" content=\"");
        html.append_view(gen.config.author.to_view());
        html.append_view("\">");
    }
    if(gen.config.keywords.size() > 0) {
        html.append_view("\n\t<meta name=\"keywords\" content=\"");
        html.append_view(gen.config.keywords.to_view());
        html.append_view("\">");
    }
    
    // Open Graph Meta Tags
    html.append_view("\n\t<meta property=\"og:title\" content=\"");
    html.append_view(&title);
    html.append_view(" - ");
    html.append_view(gen.config.site_name.to_view());
    html.append_view("\">");
    if(gen.config.description.size() > 0) {
        html.append_view("\n\t<meta property=\"og:description\" content=\"");
        html.append_view(gen.config.description.to_view());
        html.append_view("\">");
    }
    html.append_view("\n\t<meta property=\"og:type\" content=\"website\">");
    if(gen.config.logo_path.size() > 0) {
        var logo_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(gen.config.logo_path.data(), &raw mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("\n\t<meta property=\"og:image\" content=\"");
            var rel_path = get_relative_path_to_root(relative_depth)
            html.append_view(rel_path.to_view());
            html.append_view(std::string_view(&raw logo_name_buf[0], len));
            html.append_view("\">");
        }
    }
    
    // Twitter Card Meta Tags
    html.append_view("\n\t<meta name=\"twitter:card\" content=\"summary\">\n\t<meta name=\"twitter:title\" content=\"");
    html.append_view(&title);
    html.append_view(" - ");
    html.append_view(gen.config.site_name.to_view());
    html.append_view("\">");
    if(gen.config.description.size() > 0) {
        html.append_view("\n\t<meta name=\"twitter:description\" content=\"");
        html.append_view(gen.config.description.to_view());
        html.append_view("\">");
    }
    if(gen.config.logo_path.size() > 0) {
        var logo_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(gen.config.logo_path.data(), &raw mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("\n\t<meta name=\"twitter:image\" content=\"");
            var rel_path = get_relative_path_to_root(relative_depth)
            html.append_view(rel_path.to_view());
            html.append_view(std::string_view(&raw logo_name_buf[0], len));
            html.append_view("\">");
        }
    }
    
    html.append_view("""
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Sora:wght@400;500;600;700&family=IBM+Plex+Mono:wght@400;500;600&family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>""");
    html.append_view(get_default_css());
    html.append_view("</style>");
    
    // Inject Theme Init Script immediately to prevent flash
    html.append_view("<script>");
    html.append_view(get_theme_init_js());
    html.append_view("</script>");
    
    // Set root path for search
    html.append_view("<script>window.rootPath = \"");
    var rel_path = get_relative_path_to_root(relative_depth)
    html.append_view(rel_path.to_view());
    html.append_view("\";</script>");
    
    // Add Search Index
    html.append_view("<script src=\"");
    html.append_view(rel_path.to_view());
    html.append_view("search_index.js\"></script>");
    
    // Syntax Highlighting
    var prism_includes = get_prism_includes(gen.config)
    html.append_view(prism_includes.to_view());
    
    html.append_view("""
</head>
<body>
    <header class="header">
        <button id="menu-toggle" class="menu-toggle" aria-label="Toggle Menu">
            <svg width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M3 12h18M3 6h18M3 18h18"/>
            </svg>
        </button>
        <a href=""");
    html.append('"');
    html.append_view(rel_path.to_view());
    html.append_view("index.html");
    html.append('"');
    html.append_view(""" class="header-brand">""");
    
    // Logo - use image if configured, otherwise SVG
    if(gen.config.logo_path.size() > 0) {
        var logo_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(gen.config.logo_path.data(), &raw mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("<img src=\"");
            html.append_view(rel_path.to_view());
            html.append_view(std::string_view(&raw logo_name_buf[0], len));
            html.append_view("\" alt=\"Logo\" style=\"height:48px;width:auto;margin-right:8px\">");
        }
    } else {
        html.append_view("""
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M12 2L2 7l10 5 10-5-10-5z"/>
                <path d="M2 17l10 5 10-5"/>
                <path d="M2 12l10 5 10-5"/>
            </svg>""");
    }
    html.append_view(gen.config.site_name.to_view());
    html.append_view("""
        </a>
        <div class="header-spacer"></div>
        <div class="search-container">
            <svg class="search-icon" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="11" cy="11" r="8"/>
                <path d="M21 21l-4.35-4.35"/>
            </svg>
            <input type="text" id="search-input" class="search-input" placeholder="Search documentation...">
            <span class="search-kbd">Ctrl+K</span>
        </div>
        <div class="header-controls">
            <select id="theme-select" class="theme-select"></select>
        </div>
    </header>
    
    <div class="app-layout">
        <nav class="sidebar">
            <ul class="sidebar-list">
""");
    
    // Render Sidebar
    var i = 0u;
    while(i < gen.summary.items.size()) {
        var sb_item = render_sidebar_item(gen.summary.items.get(i), current_md_path, relative_depth)
        html.append_view(sb_item.to_view());
        i++;
    }
    
    html.append_view("""
            </ul>
        </nav>
        
        <main class="content">
""");
    // Breadcrumbs
    var crumbs = breadcrumb_html(page_item, &gen.summary.items, relative_depth);
    html.append_string(crumbs.copy());

    // On-this-page TOC (h2/h3)
    var content_copy = std::string(content);
    var toc = build_toc(content_copy.to_view(), relative_depth);
    if(toc.size() > 0) {
        html.append_view("<details class=\"toc\"><summary>On this page</summary>");
        html.append_string(toc.copy());
        html.append_view("</details>");
    }

    html.append_view("<div class=\"page-body\">");
    html.append_view(&content); // Already HTML from md::to_html
    html.append_view("</div>");

    // Prev / Next pager
    var pager = pager_html(gen, page_item, relative_depth);
    if(pager.size() > 0) {
        html.append_string(pager.copy());
    }

    // Footer
    var rel_p = get_relative_path_to_root(relative_depth);
    html.append_view("<footer class=\"doc-footer\"><span>");
    html.append_view(gen.config.site_name.to_view());
    html.append_view("</span><span><a href=\"");
    html.append_view(rel_p.to_view());
    html.append_view("index.html\">Home</a> &nbsp;&middot;&nbsp; <a href=\"https://github.com/chemicallang/chemical\" target=\"_blank\">GitHub</a> &nbsp;&middot;&nbsp; <a href=\"https://playground.chemicallang.com\" target=\"_blank\">Playground</a></span></footer>");
    html.append_view("""
        </main>
    </div>
    
    <script>""");
    html.append_view(get_default_js());
    html.append_view("""</script>
</body>
</html>""");
    
    // Ensure parent dir exists
    var parent = fs::parent_path_view(output_path.to_view());
    mkdir_p(parent.to_view());

    fs::write_text_file(output_path.data(), html.data() as *u8, html.size());
}


func (gen : &mut HtmlGenerator) process_item(item : *SummaryItem) {
    printf("Processing@%p: '%s' link='%s' (link.size=%d) children=%d\n", item, item.title.c_str(), item.link.c_str(), item.link.size(), item.children.size());
    
    if(item.link.size() > 0) {
        // Depth for relative links calculation - MUST do before replace_extension which may consume the link
        var depth = 0;
        var i = 0u;
        while(i < item.link.size()) {
            if(item.link.data()[i] == '/') depth++;
            i++;
        }
        
        // Read MD file - Link is relative to root (where SUMMARY.md is)
        var path = gen.config.root_path.copy();
        path.append('/');
        path.append_view(item.link.to_view());
        
        // Compute output path - use copy to avoid moving the original link
        var out_path = gen.config.build_dir.copy();
        out_path.append('/');
        var out_rel = replace_extension(&item.link, ".md", ".html");
        out_path.append_view(out_rel.to_view());
        
        var highlighter : (lang : std::string_view, code : std::string_view) => std::string = (lang, code) => {
            return highlight_wrapper(lang, code);
        };

        var link_rewriter : (url : std::string_view) => std::string = (url) => {
            var url_str = std::string(url);
            return replace_extension(&url_str, ".md", ".html");
        };
        // If config is empty, maybe we should default to all? 
        // No, user requirement implies explicit list.
        
        var content_html = md::file_to_html(path.data(), highlighter, link_rewriter);

        if(content_html is std::Result.Ok) {
            printf("Generating: %s -> %s (depth=%d)\n", item.link.c_str(), out_path.c_str(), depth);
            var Ok(html) = content_html else unreachable;
            
            // Generate Page
            gen.generate_page(item.title.to_view(), html.to_view(), out_path, depth, item.link.to_view(), item);
            
            // Add to Sitemap list (the relative URL as seen on web)
            gen.generated_urls.push_back(out_rel.copy());

            // Add to Search Index
            if(gen.search_index.size() > 1) { // Not first item (has '[')
                gen.search_index.append_view(",");
            }
            gen.search_index.append_view("{\"title\":\"");
            var escaped = escape_json_string(item.title.to_view())
            gen.search_index.append_view(escaped.to_view());
            gen.search_index.append_view("\",\"link\":\"");
            gen.search_index.append_view(out_rel.to_view());
            gen.search_index.append_view("\",\"snippet\":\"");
            // Strip tags and take first 200 chars for snippet
            var text = strip_tags(html.to_view());
            var d = 0;
            // Limit snippet
            // (simplified for now, full text search is heavy without better indexing hacks, but this is a start)
            var escaped2 = escape_json_string(text.to_view())
            gen.search_index.append_view(escaped2.to_view());
            // In a real impl we might want `content` field too, but user asked for "snippet" in UI..
            // Actually the UI filters on `content` so we should send content.
            gen.search_index.append_view("\",\"content\":\"");
            gen.search_index.append_view(escaped2.to_view());
            gen.search_index.append_view("\"}");

        } else {
            printf("Failed to process file: %s (Source: %s)\n", item.link.c_str(), path.c_str());
        }

    }
    
    // Process children recursively
    var i = 0u;
    while(i < item.children.size()) {
        gen.process_item(item.children.get(i));
        i++;
    }
}

func generate_sitemap(config : &DocConfig, urls : &std::vector<std::string>) {
    var sitemap_path = config.build_dir.copy();
    sitemap_path.append_view("/sitemap.xml");

    var xml = std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    xml.append_view("<urlset xmlns=\"http://www.sitemaps.org/schemas/sitemap/0.9\">\n");

    var i = 0u;
    while(i < urls.size()) {
        xml.append_view("  <url>\n    <loc>");
        
        // Base URL might or might not have trailing slash
        xml.append_view(config.base_url.to_view());
        if(!config.base_url.ends_with("/")) {
            xml.append('/');
        }
        
        xml.append_view(urls.get_ptr(i).to_view());
        xml.append_view("</loc>\n  </url>\n");
        i++;
    }
    
    xml.append_view("</urlset>\n");

    fs::write_text_file(sitemap_path.data(), xml.data() as *u8, xml.size());
    printf("Generated sitemap at %s\n", sitemap_path.c_str());
}

public func generate(config : DocConfig, summary : *Summary) {
    // Create build dir
    fs::mkdir(config.build_dir.data());
    
    var flat_pages = std::vector<*mut SummaryItem>();
    collect_pages(&summary.items, &mut flat_pages);
    
    var gen = HtmlGenerator { 
        config : &raw config,
        summary : summary, 
        search_index : std::string("["),
        generated_urls : std::vector<std::string>(),
        pages : std::vector<*mut SummaryItem>()
    };
    {
        var pi = 0u;
        while(pi < flat_pages.size()) {
            gen.pages.push_back(flat_pages.get(pi));
            pi++;
        }
    }
    
    var i = 0u;
    while(i < summary.items.size()) {
        gen.process_item(summary.items.get(i));
        i++;
    }
    
    // Close index and write
    gen.search_index.append_view("]");
    
    var index_path = config.build_dir.copy();
    index_path.append_view("/search_index.js");
    
    var js_content = std::string("window.searchIndex = ");
    js_content.append_view(gen.search_index.to_view());
    js_content.append_view(";");

    fs::write_text_file(index_path.data(), js_content.data() as *u8, js_content.size());
    printf("Generated search index at %s\n", index_path.c_str());

    // Copy custom index if provided
    if (config.index_path.size() > 0) {
        var out_index = config.build_dir.copy();
        out_index.append_view("/index.html");
        var custom_index = fs::copy_file(config.index_path.data(), out_index.data());
        if (custom_index is std::Result.Ok) {
            printf("Copied custom index from %s to %s\n", config.index_path.c_str(), out_index.c_str());
        } else {
            printf("Error: Could not read custom index file at %s\n", config.index_path.c_str());
        }
    }
    
    // Copy favicon if provided
    if (config.favicon_path.size() > 0) {
        var favicon_src = config.favicon_path.copy();
        
        var favicon_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(config.favicon_path.data(), &raw mut favicon_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            var favicon_dest = config.build_dir.copy();
            favicon_dest.append('/');
            favicon_dest.append_view(std::string_view(&raw favicon_name_buf[0], len));
            
            var result = fs::copy_file(favicon_src.data(), favicon_dest.data());
            if (result is std::Result.Ok) {
                printf("Copied favicon from %s to %s\n", favicon_src.c_str(), favicon_dest.c_str());
            } else {
                printf("Error: Could not copy favicon file from %s\n", favicon_src.c_str());
            }
        }
    }
    
    // Copy logo if provided
    if (config.logo_path.size() > 0) {
        var logo_src = config.logo_path.copy()
        
        var logo_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(config.logo_path.data(), &raw mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            var logo_dest = config.build_dir.copy();
            logo_dest.append('/');
            logo_dest.append_view(std::string_view(&raw logo_name_buf[0], len));
            
            var result = fs::copy_file(logo_src.data(), logo_dest.data());
            if (result is std::Result.Ok) {
                printf("Copied logo from %s to %s\n", logo_src.c_str(), logo_dest.c_str());
            } else {
                printf("Error: Could not copy logo file from %s\n", logo_src.c_str());
            }
        }
    }

    // Generate Sitemap if base_url is provided
    if (config.base_url.size() > 0) {
        generate_sitemap(&config, &gen.generated_urls);
    }
}

}
