// ===== HTML form bodies =====
//
// `application/x-www-form-urlencoded` is the encoding every `<form>` without an
// `enctype` produces, so a POST handler cannot do without it. The pieces already
// exist for query strings (`parse_query`, `url_decode`); this file gives them a
// name and a request-shaped front door:
//
//   var form = http::parse_form(req.body_text())
//   var q = form.get("q")
//
// `Form` owns its decoded keys and values: the views `get()` hands out point into
// the form, not into the request buffer, so they stay valid even after the body is
// consumed. Multipart (`multipart/form-data`, the file-upload encoding) is a later
// addition — `parse_form` does not guess at it; check
// `req.is_content_type("application/x-www-form-urlencoded")` first when a handler
// accepts both.
public namespace http {

using std::Option;
using std::pair;
using std::string;
using std::string_view;
using std::vector;

/// A decoded urlencoded body, in submission order (so two fields with the same
/// name keep their order and `get` returns the first).
@direct_init
public struct Form {
    private var fields : vector<pair<string, string>>

    @constructor
    func constructor() {
        return Form {
            fields : vector<pair<string, string>>()
        }
    }

    /// Number of fields.
    public func size(&self) : ubigint {
        return fields.size()
    }

    public func is_empty(&self) : bool {
        return fields.size() == 0u
    }

    /// The first value for `name`, or `None`. Prefer this over `text` when
    /// "missing" and "present but empty" must be told apart.
    public func get(&self, name : &str) : Option<string_view> {
        var want = name.to_view()
        var i : ubigint = 0
        while(i < fields.size()) {
            var p = fields.get_ptr(i)
            var key = unsafe(p.first.to_view())
            if(key.equals(&want)) {
                return Option.Some<string_view>(unsafe(p.second.to_view()))
            }
            i = i + 1
        }
        return Option.None<string_view>()
    }

    /// The first value for `name`, or `""`.
    public func text(&self, name : &str) : string {
        var found = get(name)
        if(found is Option.Some) {
            var Some(value) = found else unsafe { unreachable }
            var out = string()
            out.append_view(&value)
            return out
        }
        return string()
    }

    public func has(&self, name : &str) : bool {
        return get(name) is Option.Some
    }

    /// The field name at `index` (for iterating a form whose fields are not known
    /// up front), or `""`.
    public func name_at(&self, index : ubigint) : string_view {
        if(index >= fields.size()) { return string_view("") }
        var p = fields.get_ptr(index)
        return unsafe(p.first.to_view())
    }

    /// The value at `index`, or `""`.
    public func value_at(&self, index : ubigint) : string_view {
        if(index >= fields.size()) { return string_view("") }
        var p = fields.get_ptr(index)
        return unsafe(p.second.to_view())
    }

    /// Internal: one decoded pair. The form owns its bytes, so a view handed out by
    /// `get()` cannot be invalidated by the request buffer being reused.
    func push(&mut self, name : string_view, value : string_view) {
        var n = string()
        n.append_view(&name)
        var v = string()
        v.append_view(&value)
        fields.push_back(pair<string, string> {
            first : n,
            second : v
        })
    }
}

/// Parse an `application/x-www-form-urlencoded` body. Malformed input never fails:
/// a pair without `=` becomes `name=""`, an empty pair is skipped, and a `%` that
/// does not introduce two hex digits is kept literally (browsers do the same).
public func parse_form(body : string_view) : Form {
    // `parse_query` already decodes `+` and `%XX`; the form re-owns the pairs so
    // its views belong to it and not to the request buffer.
    var form = Form()
    var raw = parse_query(&body)
    var i : ubigint = 0
    while(i < raw.size()) {
        var p = raw.get_ptr(i)
        var name = unsafe(p.first.to_view())
        var value = unsafe(p.second.to_view())
        form.push(name, value)
        i = i + 1
    }
    return form
}

} // namespace http
