
@no_mangle
public func transformer_main(ctx : *TransformerContext, argc : int, argv : **char) : int {
    printf("Refgen Transformer started\n");

    var output_dir = std::string("docs/api");

    // Basic argument parsing
    var i = 1;
    while (i < argc) {
        var arg = std::string_view(argv[i]);
        if (arg.equals("--output") || arg.equals("-o")) {
            if (i + 1 < argc) {
                output_dir = std::string(argv[i + 1]);
                i += 2;
                continue;
            } else {
                printf("Error: --output requires an argument\n");
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
        output_dir: output_dir.copy(),
        ctx: ctx
    };

    var j = 0u;
    while (j < deps.size()) {
        var mod = deps.get(j) as *TransformerModule;
        printf("Processing module: %s\n", mod.getName().data());
        generator.generate(mod);
        j++;
    }

    printf("Documentation generated successfully in %s\n", output_dir.data());
    return 0;
}
