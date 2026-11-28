// P1.14 host entry. Linux with GTK3 uses ui_native's real window; the headless
// loop remains the explicit opt-in (Options.headless / --ui-headless).
using std::Result;
using std::Unit;
using std::string_view;

public func <A> run_with(app : &mut A, options : Options) : Result<Unit, UiError> where A : Application {
    if(!options.headless) {
        return ui_native::run_gtk(app, options)
    }
    return run_headless(app, options)
}

// Minimal argv entry. `--ui-headless` keeps the deterministic P1.10 loop for
// tests/CI; otherwise the native host is selected on Linux.
public func <A> run(app : &mut A, argc : int, argv : **char) : int where A : Application {
    var options = Options.make()
    options.headless = false
    options.max_frames = 0u32
    var i = 1
    while(i < argc) {
        var arg = unsafe(std::string_view.make_no_len(unsafe(argv[i])))
        if(arg.equals_with_len("--ui-headless", 12u)) { options.headless = true; options.max_frames = 1u32 }
        i = i + 1
    }
    var r = run_with(app, options)
    if(r is Result.Ok) { return 0 }
    return 1
}
