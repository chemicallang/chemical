public namespace docgen {

public struct HtmlGenerator {
    var config : *DocConfig
    var summary : *Summary
    var search_index : std::string
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
    if(path.ends_with(old_ext)) {
        var str = std::string();
        str.append_view(std::string_view(path.data(), path.size() - old_ext.size()))
        str.append_view(new_ext);
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
        var link = replace_extension(link_copy, ".md", ".html");
        html.append_view("<a href=\"");
        // Prefix with relative path to root based on current page depth
        html.append_view(get_relative_path_to_root(depth).to_view());
        html.append_view(link.to_view());
        
        html.append_view("\"");
        if(item.link.equals_view(current_path)) {
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
            html.append_view(render_sidebar_item(item.children.get(i), current_path, depth).to_view());
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
        if(vec.get(i).equals_view(val)) return true;
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

func (gen : &mut HtmlGenerator) generate_page(title : std::string_view, content : std::string_view, output_path : std::string, relative_depth : int, current_md_path : std::string_view) {
    var html = std::string("""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>""");
    html.append_view(title);
    html.append_view(" - ");
    html.append_view(gen.config.site_name.to_view());
    html.append_view("""</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">""");
    
    // Favicon
    if(gen.config.favicon_path.size() > 0) {
        var favicon_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(gen.config.favicon_path.data(), &mut favicon_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("\n\t<link rel=\"icon\" type=\"image/png\" href=\"");
            html.append_view(get_relative_path_to_root(relative_depth).to_view());
            html.append_view(std::string_view(&favicon_name_buf[0], len));
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
    html.append_view(title);
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
        var r = fs::basename(gen.config.logo_path.data(), &mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("\n\t<meta property=\"og:image\" content=\"");
            html.append_view(get_relative_path_to_root(relative_depth).to_view());
            html.append_view(std::string_view(&logo_name_buf[0], len));
            html.append_view("\">");
        }
    }
    
    // Twitter Card Meta Tags
    html.append_view("\n\t<meta name=\"twitter:card\" content=\"summary\">\n\t<meta name=\"twitter:title\" content=\"");
    html.append_view(title);
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
        var r = fs::basename(gen.config.logo_path.data(), &mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("\n\t<meta name=\"twitter:image\" content=\"");
            html.append_view(get_relative_path_to_root(relative_depth).to_view());
            html.append_view(std::string_view(&logo_name_buf[0], len));
            html.append_view("\">");
        }
    }
    
    html.append_view("""
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>""");
    html.append_view(get_default_css());
    html.append_view("</style>");
    
    // Inject Theme Init Script immediately to prevent flash
    html.append_view("<script>");
    html.append_view(get_theme_init_js());
    html.append_view("</script>");
    
    // Set root path for search
    html.append_view("<script>window.rootPath = \"");
    html.append_view(get_relative_path_to_root(relative_depth).to_view());
    html.append_view("\";</script>");
    
    // Add Search Index
    html.append_view("<script src=\"");
    html.append_view(get_relative_path_to_root(relative_depth).to_view());
    html.append_view("search_index.js\"></script>");
    
    // Syntax Highlighting
    html.append_view(get_prism_includes(gen.config).to_view());
    
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
    html.append_view(get_relative_path_to_root(relative_depth).to_view());
    html.append_view("index.html");
    html.append('"');
    html.append_view(""" class="header-brand">""");
    
    // Logo - use image if configured, otherwise SVG
    if(gen.config.logo_path.size() > 0) {
        var logo_name_buf : [fs::PATH_MAX_BUF]char;
        var r = fs::basename(gen.config.logo_path.data(), &mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            html.append_view("<img src=\"");
            html.append_view(get_relative_path_to_root(relative_depth).to_view());
            html.append_view(std::string_view(&logo_name_buf[0], len));
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
        html.append_view(render_sidebar_item(gen.summary.items.get(i), current_md_path, relative_depth).to_view());
        i++;
    }
    
    html.append_view("""
            </ul>
        </nav>
        
        <main class="content">
""");
    html.append_view(content); // Already HTML from md::to_html
    html.append_view("""
        </main>
    </div>
    
    <script>""");
    html.append_view(get_default_js());
    html.append_view("""</script>
</body>
</html>""");
    
    // Ensure parent dir exists
    var parent = fs::parent_path(output_path.to_view());
    mkdir_p(parent.to_view());
    
    fs::write_to_file(output_path.data(), html.data());
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
        var out_rel = replace_extension(item.link, ".md", ".html");
        out_path.append_view(out_rel.to_view());
        
        var highlighter : (lang : std::string_view, code : std::string_view) => std::string = (lang, code) => {
            return highlight_wrapper(lang, code);
        };

        var link_rewriter : (url : std::string_view) => std::string = (url) => {
            var url_str = std::string(url);
            return replace_extension(url_str, ".md", ".html");
        };
        // If config is empty, maybe we should default to all? 
        // No, user requirement implies explicit list.
        
        var content_html = md::file_to_html(path.data(), highlighter, link_rewriter);

        if(content_html is std::Result.Ok) {
            printf("Generating: %s -> %s (depth=%d)\n", item.link.c_str(), out_path.c_str(), depth);
            var Ok(html) = content_html else unreachable;
            
            // Generate Page
            gen.generate_page(item.title.to_view(), html.to_view(), out_path, depth, item.link.to_view());
            
            // Add to Search Index
            if(gen.search_index.size() > 1) { // Not first item (has '[')
                gen.search_index.append_view(",");
            }
            gen.search_index.append_view("{\"title\":\"");
            gen.search_index.append_view(escape_json_string(item.title.to_view()).to_view());
            gen.search_index.append_view("\",\"link\":\"");
            gen.search_index.append_view(out_rel.to_view());
            gen.search_index.append_view("\",\"snippet\":\"");
            // Strip tags and take first 200 chars for snippet
            var text = strip_tags(html.to_view());
            var d = 0;
            // Limit snippet
            // (simplified for now, full text search is heavy without better indexing hacks, but this is a start)
            gen.search_index.append_view(escape_json_string(text.to_view()).to_view()); 
            // In a real impl we might want `content` field too, but user asked for "snippet" in UI..
            // Actually the UI filters on `content` so we should send content.
            gen.search_index.append_view("\",\"content\":\"");
             gen.search_index.append_view(escape_json_string(text.to_view()).to_view()); 
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

public func generate(config : DocConfig, summary : *Summary) {
    // Create build dir
    fs::mkdir(config.build_dir.data());
    
    var gen = HtmlGenerator { config : &config, summary : summary, search_index : std::string("[") };
    
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
    
    fs::write_to_file(index_path.data(), js_content.data());
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
        var r = fs::basename(config.favicon_path.data(), &mut favicon_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            var favicon_dest = config.build_dir.copy();
            favicon_dest.append('/');
            favicon_dest.append_view(std::string_view(&favicon_name_buf[0], len));
            
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
        var r = fs::basename(config.logo_path.data(), &mut logo_name_buf[0], fs::PATH_MAX_BUF as size_t);
        if(r is std::Result.Ok) {
            var Ok(len) = r else unreachable;
            var logo_dest = config.build_dir.copy();
            logo_dest.append('/');
            logo_dest.append_view(std::string_view(&logo_name_buf[0], len));
            
            var result = fs::copy_file(logo_src.data(), logo_dest.data());
            if (result is std::Result.Ok) {
                printf("Copied logo from %s to %s\n", logo_src.c_str(), logo_dest.c_str());
            } else {
                printf("Error: Could not copy logo file from %s\n", logo_src.c_str());
            }
        }
    }
}

}
