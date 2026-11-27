// P0.3b benchmark scaffold. Real renderer/layout/timing is not implemented;
// this emits deterministic counters for the scene manifests and labels the
// renderer as `none`. P1.10 replaces the runner with real frame timing.
using std::string_view;

func eq(a : *char, b : *char) : bool {
    unsafe { return strcmp(a, b) == 0 }
}

public func main(argc : int, argv : **char) : int {
    var scenario = "empty"
    var hash = ""
    var json = false
    var i = 1
    while(i < argc) {
        var arg = unsafe(argv[i])
        if(eq(arg, "--json")) {
            json = true
        } else if(eq(arg, "--scenario")) {
            i = i + 1
            if(i >= argc) { println("ui_bench: --scenario needs a name"); return 2 }
            scenario = unsafe(argv[i])
        } else if(eq(arg, "--scene-hash")) {
            i = i + 1
            if(i >= argc) { println("ui_bench: --scene-hash needs a value"); return 2 }
            hash = unsafe(argv[i])
        } else if(eq(arg, "--help")) {
            println("ui_bench --scenario empty|static-1k|list-100k [--json] [--scene-hash H]")
            return 0
        } else {
            println("ui_bench: unknown argument")
            return 2
        }
        i = i + 1
    }
    var nodes : u32 = 0u32
    var commands : u32 = 0u32
    var frames : u32 = 1u32
    if(eq(scenario, "empty")) {
        nodes = 0u32
        commands = 0u32
        frames = 1u32
    } else if(eq(scenario, "static-1k")) {
        nodes = 1000u32
        commands = 1000u32
        frames = 61u32
    } else if(eq(scenario, "list-100k")) {
        nodes = 100000u32
        commands = 100000u32
        frames = 2000u32
    } else {
        println("ui_bench: unknown scenario (P0.3b has empty|static-1k|list-100k)")
        return 2
    }
    if(json) {
        var buf : [512]char
        unsafe {
            sprintf(&raw mut buf[0],
                    "{\"scenario\":\"%s\",\"profile\":\"reference-linux-x86_64\",\"build\":\"debug_quick\",\"renderer\":\"none\",\"scene_hash\":\"%s\",\"frames\":%u,\"counters\":{\"nodes\":%u,\"commands\":%u,\"allocs\":0,\"frames\":%u},\"notes\":\"scaffold counters; no real timing\"}",
                    scenario, hash, frames, nodes, commands, frames)
        }
        unsafe { printf("%s\n", &raw mut buf[0]) }
    } else {
        println("ui_bench scaffold scenario (no real timing)")
        unsafe { printf("%s\n", scenario) }
        println("ui_bench PASS")
    }
    return 0
}
