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

public func build_docs(root_path : *char, output_path : *char) : int {
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
    
    printf("Compiling documentation for: %s\n", summary.title.c_str());
    
    var config = DocConfig {
        root_path : base_dir.copy(),
        build_dir : std::string(),
        site_name : summary.title.copy()
    };

    var out_view = std::string_view(output_path)
    if(out_view.equals("book")) {
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
        printf("Usage: docgen <root_dir> [output_dir]\n");
        return 1;
    }
    
    var root = *argv_offset(argv, 1);
    var output = "book";
    if(argc >= 3) {
        output = *argv_offset(argv, 2);
    }
    
    return build_docs(root, output);
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
