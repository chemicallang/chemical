// A library has no `main`. Its JS build exposes the public functions through
// each emitted module's `$exports` table; the gate imports the module from Node
// and calls them by discovering their mangled keys.

// Not public, but reachable because `library_answer` calls it: it must be in
// `$exports` (the full symbol table) and absent from `$api` (the public surface).
func library_hidden_helper() : int {
    return 40
}

public func library_answer() : int {
    return library_hidden_helper() + 2
}

public func library_sum(limit : int) : int {
    var total : int = 0
    var i : int = 1
    while(i <= limit) {
        total = total + i
        i = i + 1
    }
    return total
}

// An async public function is part of the library's API: its ramp/poll pair
// must survive reachability from the root set.
public async func library_async_answer() : int {
    return 43
}
