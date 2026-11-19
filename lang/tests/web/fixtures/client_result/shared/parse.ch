// Shared by the client (JavaScript) and server (native) job. A fallible parse is
// exactly the kind of function whose result must not be dropped: the client
// policy (`WEB-CLIENT-001`) rejects a discarded `std::Result`, and the server
// is still allowed to ignore one it has already recorded.

public func shared_parse(text : std::string_view) : std::Result<int, std::string> {
    if(text.size() == 0) {
        return std::Result.Err(std::string("empty"))
    }
    return std::Result.Ok(7)
}
