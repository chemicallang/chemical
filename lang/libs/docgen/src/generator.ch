public namespace docgen {

public struct HtmlGenerator {
    var config : DocConfig
    var summary : *Summary
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

// Recursively render sidebar
func render_sidebar_item(item : *SummaryItem, current_path : std::string_view, depth : int) : std::string {
    var html = std::string("<li class=\"sidebar-item\">");
    
    if(item.link.size() > 0) {
        // Fix link extension .md -> .html
        var link = replace_extension(item.link, ".md", ".html");
        html.append_view("<a href=\"");
        // Need to handle relative paths properly!
        // For now assuming summary links are valid relative to root
        // If current page is nested, we need to adjust
        // Simplify: Generate absolute-like relative paths? No.
        // We will just put the link as is if it starts with http, otherwise we rely on correct relative structure.
        // But if I am in a subdir, I need to go up?
        // Wait, sidebar is static for all pages usually, or adapted?
        // Ideally adapted. 
        // Let's assume we pass a root_prefix to sidebar generator.
        
        // Actually simplest is to compute depth of current page and prefix all root-relative links.
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
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>""");
    html.append_view(get_default_css());
    html.append_view("""</style>
</head>
<body>
    <header class="header">
        <a href=""");
    html.append('"');
    html.append_view(get_relative_path_to_root(relative_depth).to_view());
    html.append_view("""introduction.html" class="header-brand">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <path d="M12 2L2 7l10 5 10-5-10-5z"/>
                <path d="M2 17l10 5 10-5"/>
                <path d="M2 12l10 5 10-5"/>
            </svg>
            """);
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
            <button id="dark-toggle" class="theme-toggle" title="Toggle dark mode">
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="5"/>
                    <path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/>
                </svg>
            </button>
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
        // Read MD file
        // Link is relative to root (where SUMMARY.md is)
        var path = gen.config.root_path.copy();
        path.append('/');
        path.append_view(item.link.to_view());
        
        // Compute output path
        var out_path = gen.config.build_dir.copy();
        out_path.append('/');
        var out_rel = replace_extension(std::replace(item.link, std::string()), ".md", ".html");
        out_path.append_view(out_rel.to_view());
        
        // Depth for relative links calculation
        var depth = 0;
        var i = 0u;
        while(i < item.link.size()) {
            if(item.link.data()[i] == '/') depth++;
            i++;
        }
        
        var content_html = md::file_to_html(path.data());

        if(content_html is std::Result.Ok) {
            printf("Generating: %s -> %s\n", item.link.c_str(), out_path.c_str());
            var Ok(html) = content_html else unreachable;
            gen.generate_page(item.title.to_view(), html.to_view(), out_path, depth, item.link.to_view());

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
    
    var gen = HtmlGenerator { config : config, summary : summary };
    
    // Generate index.html (Summary or First Item)
    // If there is an item called 'Introduction' or first item, we copy it to index.html too?
    // Or we stick to exact mapping.
    // Usually index.html is needed.
    // If SUMMARY references README.md, that becomes index.html if we map correctly?
    // We will just process items.
    
    var i = 0u;
    while(i < summary.items.size()) {
        gen.process_item(summary.items.get(i));
        i++;
    }
}

}
