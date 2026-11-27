// Only `used_component` (src/used.ch) is referenced from main. `unused_*`
// functions model components that an app imports but never mounts; if they are
// emitted into the binary, every kit import would cost code size.
public func unused_component_a(x : int) : int {
    var s = 0
    var i = 0
    while(i < x) {
        s = s + i * 3
        i = i + 1
    }
    return s
}

public func unused_component_b(x : int) : int {
    var v = std::vector<int>()
    var i = 0
    while(i < x) {
        v.push(i * 7)
        i = i + 1
    }
    return v.size() as int
}

public func unused_component_c(s : std::string) : int {
    var out = std::string()
    out.append_view(s.to_view())
    return out.size() as int
}
