@no_mangle
public func transformer_main(ctx : *TransformerContext, argc : int, argv : **char) : int {
    printf("Refgen Transformer started\n");

    var output_dir = std::string("./docs");
    var github_links = false;
    var git_ref = std::string("main");
    var no_search = false;
    var base_url = std::string("https://api.chemicallang.com");

    // Basic argument parsing
    var i = 1;
    while (i < argc) {
        var arg = std::string_view(argv[i]);
        if (arg.equals("--output") || arg.equals("-o")) {
            if (i + 1 < argc) {
                output_dir = std::string(argv[i+1]);
                i += 2;
                continue;
            } else {
                printf("Error: --output requires an argument\n");
                return 1;
            }
        } else if (arg.equals("--github-links")) {
            github_links = true;
            i++;
            continue;
        } else if (arg.equals("--git-ref")) {
            if (i + 1 < argc) {
                git_ref = std::string(argv[i+1]);
                i += 2;
                continue;
            } else {
                printf("Error: --git-ref requires an argument\n");
                return 1;
            }
        } else if (arg.equals("--no-search")) {
            no_search = true;
            i++;
            continue;
        } else if (arg.equals("--base-url")) {
            if (i + 1 < argc) {
                base_url = std::string(argv[i+1]);
                i += 2;
                continue;
            } else {
                printf("Error: --base-url requires an argument\n");
                return 1;
            }
        }
        i++;
    }

    // Parse the target module with comment tokens preserved
    if (!ctx.parseTarget(true)) {
        printf("Failed to parse target module\n");
        return 1;
    }

    // Analyze the target module
    if (!ctx.analyzeTarget()) {
        printf("Failed to analyze target module\n");
        return 1;
    }

    // Get the target job
    var job = ctx.getTargetJob();
    printf("Generating documentation for job: %s\n", job.getName().data());

    // Get sorted dependencies
    var deps = ctx.getFlattenedModules();

    var generator = refgen::Generator {
        output_dir = output_dir.copy(),
        ctx = ctx,
        github_links = github_links,
        git_ref = git_ref.copy(),
        no_search = no_search,
        base_url = base_url.copy(),
        index = std::vector<refgen::SymbolInfo>(),
        sidebar_cache = std::unordered_map<std::string, std::string>()
    };

    // Pass 1: Indexing
    var j = 0u;
    while (j < deps.size()) {
        var mod = deps.get(j) as *TransformerModule;
        generator.index_module(mod);
        j++;
    }

    // Pass 2: Generation
    j = 0u;
    while (j < deps.size()) {
        var mod = deps.get(j) as *TransformerModule;
        generator.generate(mod);
        j++;
    }

    generator.finish();

    printf("Documentation generated successfully in %s\n", output_dir.data());
    return 0;
}
