// The client entry point. `--web`'s client pass emits this module (and the
// `shared` logic it calls) as ES modules; `$run()` calls `main` and returns its
// exit code, so the same code runs here and in a browser bundle.
//
// `main` prints only what the gate compares: the value it computed through
// `shared`, plus a client-only answer that must never leak into the server job.
public func main() : int {
    var text = shared_label("forty-two")
    var total = shared_add(20, 22)
    unsafe {
        printf("client: %s (%d)\n", text.to_view().data(), total)
    }
    return 0
}

public func client_version() : int {
    return 7
}
