// P1.10 headless host entry. Real native/web windows arrive in P1.14/P3.x.
using std::Result;
using std::Unit;
using std::string_view;
public func <A> run_with(app : &mut A, options : Options) : Result<Unit, UiError> where A : Application {
    if(!options.headless) {
        return Result.Err<Unit,UiError>(UiError.make(ErrorCode.NotImplemented, "ui_host window host is not implemented before P1.14"))
    }
    return run_headless(app, options)
}

// Minimal argv entry: P1.10 supports only the headless loop. A real window
// facade is deliberately not faked here.
public func <A> run(app : &mut A, argc : int, argv : **char) : int where A : Application {
    var options = Options.make()
    options.headless = true
    options.max_frames = 1u32
    var i = 1
    while(i < argc) {
        var arg = unsafe(std::string_view.make_no_len(unsafe(argv[i])))
        if(arg.equals_with_len("--ui-headless", 12u)) { options.headless = true }
        i = i + 1
    }
    var r = run_with(app, options)
    if(r is Result.Ok) { return 0 }
    return 1
}
