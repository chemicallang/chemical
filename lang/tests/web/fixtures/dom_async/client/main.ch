// The client entry point, written as an `async func`: `await` on a host `fetch`
// suspends the body on the real promise the host returns, and the bundle's `$run`
// resolves only after it settles.
//
// The three outcomes an awaited request has — a response, an error status, and a
// network failure that rejects — are all awaited here.
using std::string_view;

async func load_status(url : string_view) : int {
    var status = await dom::fetch_status(url)
    return status
}

public async func main() : int {
    var ok = await load_status(string_view("/api/ok"))
    unsafe { printf("dom async ok=%d\n", ok) }

    var missing = await load_status(string_view("/api/missing"))
    unsafe { printf("dom async missing=%d\n", missing) }

    // A rejected request resolves to 0 instead of rejecting the future.
    var down = await load_status(string_view("/api/down"))
    unsafe { printf("dom async down=%d\n", down) }
    return 0
}
