// The click counter's rule, written once and used by both jobs.
public func shared_next(count : int) : int {
    return count + 1
}

public func shared_label(count : int) : std::string {
    var out = std::string()
    out.append_view("count=")
    var n = count
    if(n == 0) {
        out.append_view("0")
        return out
    }
    var digits = std::string()
    while(n > 0) {
        digits.append(((n % 10) + 48) as char)
        n = n / 10
    }
    var i = digits.size()
    while(i > 0) {
        i = i - 1
        out.append(digits.at(i))
    }
    return out
}

// The DOM stores text, so the counter reads its own previous value back from the
// page: one implementation of the rule, no hidden state on either side.
public func shared_parse_int(text : std::string_view) : int {
    var value = 0
    var i : ubigint = 0
    while(i < text.size()) {
        var c = text.at(i)
        if(c >= '0' && c <= '9') {
            value = value * 10 + ((c as int) - 48)
        }
        i = i + 1
    }
    return value
}
