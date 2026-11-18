// The server side renders the page the client script hydrates. It shares the
// counter rule with the client through `shared`, so the two agree by construction.
public func route_index() : std::string {
    var out = std::string()
    out.append_view("<!DOCTYPE html><html><body><div id=\"app\"><span id=\"greeting\" data-role=\"hello\">hi</span><input id=\"name\"><button id=\"count-button\">count</button><span id=\"count\">0</span></div><script type=\"module\" src=\"assets/client.mjs\"></script></body></html>")
    return out
}
