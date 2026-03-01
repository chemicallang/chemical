public namespace docgen {

// Helper for recursive mkdir
func mkdir_p(path : std::string_view) {
    if(path.size() == 0) return;

    var temp = std::string();
    var i = 0u;
    while(i < path.size()) {
        const c = path.data()[i];
        if(c == '/' || c == '\\') {
            if(temp.size() > 0) {
                fs::mkdir(temp.data());
            }
        }
        temp.append(c);
        i++;
    }
    if(temp.size() > 0) {
        fs::mkdir(temp.data());
    }
}

public func build_docs(root_path : *char, output_path : *char, index_path : *char, syntax_args : *char, explicit_output : bool, favicon : *char, logo : *char, description : *char, author : *char, keywords : *char) : int {
    var root_str = std::string(root_path);
    var summary_path = std::string();
    var base_dir = std::string();

    var root_view = root_str.to_view();
    if(root_view.ends_with("SUMMARY.md")) {
        summary_path = root_str.copy();
        // Strip SUMMARY.md from root
        var len = root_str.size();
        var i = len - 1;
        while(i > 0) {
            const c = root_str.data()[i];
            if(c == '/' || c == '\\') {
                 base_dir = std::string(std::string_view(root_str.data(), i));
                 break;
            }
            i--;
        }
        if(i == 0) {
             base_dir = std::string(".");
        }
    } else {
        base_dir = root_str.copy();
        summary_path = base_dir.copy();
        summary_path.append_view("/SUMMARY.md");
    }
    
    // Parse Summary
    printf("Resolved Root: %s\n", base_dir.c_str());
    printf("Resolved Summary Path: %s\n", summary_path.c_str());
    
    var summary = parse_summary(summary_path.to_view());
    if(summary == null) {
        printf("Error: Could not parse SUMMARY.md at %s\n", summary_path.c_str());
        return 1;
    }

    printf("Summary:\n");
    print_summary(*summary);
    printf("Compiling documentation for: %s\n", summary.title.c_str());

    var config = DocConfig {
        root_path : base_dir.copy(),
        build_dir : std::string(),
        site_name : summary.title.copy(),
        index_path : (if(index_path != null) std::string(index_path) else std::string()),
        syntax_highlights : std::vector<std::string>(),
        favicon_path : (if(favicon != null) std::string(favicon) else std::string()),
        logo_path : (if(logo != null) std::string(logo) else std::string()),
        description : (if(description != null) std::string(description) else std::string()),
        author : (if(author != null) std::string(author) else std::string()),
        keywords : (if(keywords != null) std::string(keywords) else std::string())
    };

    if(syntax_args != null) {
        var s = std::string_view(syntax_args);
        var start = 0u;
        var i = 0u;
        while(i < s.size()) {
            if(s.data()[i] == ',') {
                 config.syntax_highlights.push_back(std::string(std::string_view(s.data() + start, i - start)));
                 start = i + 1u;
            }
            i++;
        }
        if(start < s.size()) {
            config.syntax_highlights.push_back(std::string(std::string_view(s.data() + start, s.size() - start)));
        }
    }

    var out_view = std::string_view(output_path)
    if(!explicit_output && out_view.equals("book")) {
         // Default case: make it relative to root
         config.build_dir = base_dir.copy();
         config.build_dir.append_view("/book");
    } else {
         // User provided path, keep as is (relative to CWD)
         config.build_dir = std::string(output_path);
    }
    
    // Ensure build dir exists (recursive)
    printf("Output Directory: %s\n", config.build_dir.c_str());
    mkdir_p(config.build_dir.to_view());
    
    generate(config, summary);
    
    printf("Documentation built successfully\n");
    return 0;
}

public func main(argc : int, argv : **char) : int {
    if(argc < 2) {
        printf("Usage: docgen <root_dir> [-o output_dir] [--index index.html] [--syntax-highlight list] [--favicon path] [--logo path] [--description text] [--author name] [--keywords keywords]\n");
        return 1;
    }
    
    var root = *argv_offset(argv, 1);
    var output = "book";
    var index_file : *char = null;
    var explicit_output = false;
    var syntax_args : *char = null;
    var favicon : *char = null;
    var logo : *char = null;
    var description : *char = null;
    var author : *char = null;
    var keywords : *char = null;

    var i = 2;
    while(i < argc) {
        const arg_str = *argv_offset(argv, i);
        var arg = std::string_view(arg_str, strlen(arg_str));
        if(arg.equals("--syntax-highlight")) {
            if(i + 1 < argc) {
                syntax_args = *argv_offset(argv, i + 1);
                i++;
            }
        } else if(arg.equals("--index")) {
            if(i + 1 < argc) {
                index_file = *argv_offset(argv, i + 1);
                i++;
            }
        } else if(arg.equals("-o") || arg.equals("--output")) {
            if(i + 1 < argc) {
                output = *argv_offset(argv, i + 1);
                explicit_output = true;
                i++;
            }
        } else if(arg.equals("--favicon")) {
            if(i + 1 < argc) {
                favicon = *argv_offset(argv, i + 1);
                i++;
            }
        } else if(arg.equals("--logo")) {
            if(i + 1 < argc) {
                logo = *argv_offset(argv, i + 1);
                i++;
            }
        } else if(arg.equals("--description")) {
            if(i + 1 < argc) {
                description = *argv_offset(argv, i + 1);
                i++;
            }
        } else if(arg.equals("--author")) {
            if(i + 1 < argc) {
                author = *argv_offset(argv, i + 1);
                i++;
            }
        } else if(arg.equals("--keywords")) {
            if(i + 1 < argc) {
                keywords = *argv_offset(argv, i + 1);
                i++;
            }
        } else {
            // Assume it's output dir if not a flag and not already set
             if(!explicit_output) { 
                 output = arg.data() as *char; 
                 explicit_output = true;
             }
        }
        i++;
    }
    
    return build_docs(root, output, index_file, syntax_args, explicit_output, favicon, logo, description, author, keywords);
}

func argv_offset(argv : **char, offset : int) : **char {
    var p = argv as *mut *char;
    var i = 0;
    while(i < offset) {
        unsafe { p = p + 1; }
        i++;
    }
    return p;
}

}

protected func main(argc : int, argv : **char) : int {
    return docgen::main(argc, argv)
}