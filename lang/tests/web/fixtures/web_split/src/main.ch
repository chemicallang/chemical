// The server entry point: only compiled in the server pass (`source "src" if server`).
//
// It proves the two sides really are separate jobs: the server can use `server`
// (native `getenv`), and it cannot see `client` at all — uncommenting the line
// below is a compile error, not a runtime failure.
public func main() : int {
    var count = route_count()
    unsafe {
        printf("server: %d\n", count)
    }
    // var v = client_version()   // error: 'client_version' is not in this job
    return 0
}
