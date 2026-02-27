
public func transformer_main(ctx : *TransformerContext, argc : int, argv : **char) : int {
    printf("Refgen Transformer started\n");

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
        output_dir: std::string("docs/api"),
        ctx: ctx
    };

    var i = 0u;
    while (i < deps.size()) {
        var mod = deps.get(i) as *TransformerModule;
        printf("Processing module: %s\n", mod.getName().data());
        generator.generate(mod);
        i++;
    }

    printf("Documentation generated successfully in docs/api\n");
    return 0;
}
