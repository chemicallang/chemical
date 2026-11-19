// Server-only code. It discards the same `std::Result` the client may not, which
// proves the policy is scoped to the client job (`target_data.client`) instead of
// being a general "must use every Result" rule.
public func main() : int {
    shared_parse("")
    return 0
}
