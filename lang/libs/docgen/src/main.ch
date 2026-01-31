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
    var root = std::string(root_path);
    var summary_path = root.copy();
    summary_path.append_view("/SUMMARY.md");
    
    // Parse Summary
    var summary = parse_summary(summary_path.to_view());
    if(summary == null) {
        printf("Error: Could not parse SUMMARY.md at %s\n", summary_path.c_str());
        return 1;
    }
    
    printf("Compiling documentation for: %s\n", summary.title.c_str());
    
    var config = DocConfig {
        root_path : root,
        build_dir : std::string(output_path),
        site_name : summary.title.copy()
    };
    
    // Ensure build dir exists (recursive)
    mkdir_p(config.build_dir.to_view());
    
    generate(config, summary);
    
    printf("Documentation built successfully in %s\n", output_path);
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
