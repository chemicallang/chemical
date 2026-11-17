// NEGATIVE fixture: the client reaches a shared function that calls a native
// symbol. The client pass must fail here (via `shared_platform_label`), not
// produce a bundle.

public func main() : int {
    var label = shared_platform_label()
    unsafe { printf("client: %s\n", label.to_view().data()) }
    return 0
}
