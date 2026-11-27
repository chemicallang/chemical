// P0.3a benchmark scaffold. P0.3b adds real scenes/baselines and the counter
// harness. Until then `empty` emits deterministic counters and valid JSON.
using std::string_view;

func eq(a : *char, b : *char) : bool {
    unsafe { return strcmp(a, b) == 0 }
}

public func main(argc : int, argv : **char) : int {
    var scenario = "empty"
    var json = false
    var i = 1
    while(i < argc) {
        var arg = unsafe(argv[i])
        if(eq(arg, "--json")) {
            json = true
        } else if(eq(arg, "--scenario")) {
            i = i + 1
            if(i >= argc) {
                println("ui_bench: --scenario needs a name")
                return 2
            }
            scenario = unsafe(argv[i])
        } else if(eq(arg, "--help")) {
            println("ui_bench --scenario empty [--json]")
            return 0
        } else {
            println("ui_bench: unknown argument")
            return 2
        }
        i = i + 1
    }
    if(!eq(scenario, "empty")) {
        println("ui_bench: unknown scenario (only 'empty' exists in P0.3a)")
        return 2
    }
    if(json) {
        println("{\"scenario\":\"empty\",\"profile\":\"scaffold\",\"build\":\"debug_quick\",\"renderer\":\"none\",\"frames\":1,\"counters\":{\"nodes\":0,\"commands\":0,\"allocs\":0,\"frames\":1},\"notes\":\"P0.3a scaffold; P0.3b adds baselines\"}")
    } else {
        println("ui_bench scenario=empty nodes=0 commands=0 allocs=0 frames=1")
        println("ui_bench PASS")
    }
    return 0
}
