// Typed DOM access for client modules (proposal 0008, minimal v1 slice).
//
// `dom` is the checked alternative to `#js` for browser code: `document()`,
// `query_selector`, `create_element`, events, `fetch`, `localStorage` and `URL`,
// with ordinary Chemical types. A typo'd method is a compile error, and the JS
// backend maps each call to the browser API (see `raw` below).
//
// Client modules only. The handles name browser objects, so the server job (native)
// has no implementation for them: import `dom` from a `client` module, never from
// `shared` or `server`.
//
//     import dom in `client/chemical.mod`, then:
//
//     var doc = dom.document()
//     var button = doc.query_selector("#count").expect("button")
//     button.add_event_listener("click", on_click)
//
//     public func on_click(e : &dom::Event) {
//         e.prevent_default()
//         var label = dom::get_element_by_id(&doc, "count").expect("label")
//         label.set_text("1")
//     }
//
// Handles are host-managed: a `Document`/`Element`/`Event` is a small value holding
// an opaque id, copies are free, and there is nothing to free. The runtime keeps the
// browser object alive and never reuses an id, so a stale handle cannot silently
// alias a different element.
public namespace dom {

using std::Option;
using std::string;
using std::string_view;

// ---------------------------------------------------------------------------
// Raw host bindings.
//
// One `@extern @no_mangle` declaration per browser operation. The JS backend maps
// the symbol name to `rt.dom.*`; the C/LLVM backends have no definition for these
// (a `dom` call in a server module is a link error, which is why client modules own
// them). A handle is `0` for "no element", so an `Option` is built on the safe side
// instead of crossing the boundary as an aggregate.
// ---------------------------------------------------------------------------
public namespace raw {

    @extern @no_mangle
    func chx_dom_document() : usize

    @extern @no_mangle
    func chx_dom_body(doc : usize) : usize

    @extern @no_mangle
    func chx_dom_get_element_by_id(doc : usize, id : string_view) : usize

    @extern @no_mangle
    func chx_dom_query_selector(root : usize, selector : string_view) : usize

    @extern @no_mangle
    func chx_dom_create_element(doc : usize, tag : string_view) : usize

    @extern @no_mangle
    func chx_dom_set_text(handle : usize, text : string_view)

    @extern @no_mangle
    func chx_dom_text(handle : usize) : *char

    @extern @no_mangle
    func chx_dom_set_html(handle : usize, html : string_view)

    @extern @no_mangle
    func chx_dom_set_attribute(handle : usize, name : string_view, value : string_view)

    @extern @no_mangle
    func chx_dom_get_attribute(handle : usize, name : string_view) : *char

    @extern @no_mangle
    func chx_dom_has_attribute(handle : usize, name : string_view) : int

    @extern @no_mangle
    func chx_dom_remove_attribute(handle : usize, name : string_view)

    @extern @no_mangle
    func chx_dom_set_value(handle : usize, value : string_view)

    @extern @no_mangle
    func chx_dom_value(handle : usize) : *char

    @extern @no_mangle
    func chx_dom_append_child(parent : usize, child : usize)

    @extern @no_mangle
    func chx_dom_remove(handle : usize)

    @extern @no_mangle
    func chx_dom_focus(handle : usize)

    @extern @no_mangle
    func chx_dom_on(handle : usize, event : string_view, handler : (e : &Event) => void)

    @extern @no_mangle
    func chx_dom_prevent_default(event : usize)

    @extern @no_mangle
    func chx_dom_stop_propagation(event : usize)

    @extern @no_mangle
    func chx_dom_target(event : usize) : usize

    @extern @no_mangle
    func chx_dom_key(event : usize) : *char

    @extern @no_mangle
    func chx_dom_fetch_text(url : string_view, handler : (status : int, body : &string_view) => void)

    @extern @no_mangle
    func chx_dom_storage_get(key : string_view) : *char

    @extern @no_mangle
    func chx_dom_storage_set(key : string_view, value : string_view)

    @extern @no_mangle
    func chx_dom_storage_remove(key : string_view)

    @extern @no_mangle
    func chx_dom_url_origin(url : string_view) : *char

    @extern @no_mangle
    func chx_dom_url_pathname(url : string_view) : *char

    @extern @no_mangle
    func chx_dom_url_search_param(url : string_view, name : string_view) : *char

} // namespace raw

// A host string (`*char`) as an owned Chemical string.
func owned(p : *char) : string {
    var out = string()
    if(p == null) {
        return out
    }
    var view = unsafe(string_view(p))
    out.append_view(&view)
    return out
}

// ── Document ────────────────────────────────────────────────────────────────
/// The page's document.
@direct_init
public struct Document {
    internal var handle : usize

    /// The element with `id`, if the document has one.
    public func get_element_by_id(&self, id : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_get_element_by_id(handle, id))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// The first element matching a CSS selector.
    public func query_selector(&self, selector : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_query_selector(handle, selector))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    public func create_element(&self, tag : string_view) : Element {
        return Element { handle : unsafe(raw::chx_dom_create_element(handle, tag)) }
    }

    public func body(&self) : Element {
        return Element { handle : unsafe(raw::chx_dom_body(handle)) }
    }
}

// ── Element ─────────────────────────────────────────────────────────────────
@direct_init
public struct Element {
    internal var handle : usize

    public func set_text(&self, text : string_view) {
        unsafe { raw::chx_dom_set_text(handle, text) }
    }

    public func text(&self) : string {
        return owned(unsafe(raw::chx_dom_text(handle)))
    }

    /// Trusted markup only: the string is parsed as HTML (never pass user input).
    public func set_html(&self, html : string_view) {
        unsafe { raw::chx_dom_set_html(handle, html) }
    }

    public func set_attribute(&self, name : string_view, value : string_view) {
        unsafe { raw::chx_dom_set_attribute(handle, name, value) }
    }

    public func attribute(&self, name : string_view) : Option<string> {
        var p = unsafe(raw::chx_dom_get_attribute(handle, name))
        if(p == null) {
            return Option.None<string>()
        }
        return Option.Some<string>(owned(p))
    }

    public func has_attribute(&self, name : string_view) : bool {
        return unsafe(raw::chx_dom_has_attribute(handle, name)) != 0
    }

    public func remove_attribute(&self, name : string_view) {
        unsafe { raw::chx_dom_remove_attribute(handle, name) }
    }

    /// An `<input>`/`<textarea>` value.
    public func set_value(&self, value : string_view) {
        unsafe { raw::chx_dom_set_value(handle, value) }
    }

    public func value(&self) : string {
        return owned(unsafe(raw::chx_dom_value(handle)))
    }

    public func append_child(&self, child : &Element) {
        unsafe { raw::chx_dom_append_child(handle, child.handle) }
    }

    public func remove(&self) {
        unsafe { raw::chx_dom_remove(handle) }
    }

    public func focus(&self) {
        unsafe { raw::chx_dom_focus(handle) }
    }

    /// Register `handler` for `event` (`"click"`, `"input"`, …). The handler runs in
    /// this module's program; the listener table is owned by the browser.
    public func add_event_listener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_on(handle, event, handler) }
    }

    public func query_selector(&self, selector : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_query_selector(handle, selector))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }
}

// ── Event ───────────────────────────────────────────────────────────────────
@direct_init
public struct Event {
    internal var handle : usize

    public func prevent_default(&self) {
        unsafe { raw::chx_dom_prevent_default(handle) }
    }

    public func stop_propagation(&self) {
        unsafe { raw::chx_dom_stop_propagation(handle) }
    }

    /// The element the event started on.
    public func target(&self) : Option<Element> {
        var h = unsafe(raw::chx_dom_target(handle))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// The key of a keyboard event (empty for other events).
    public func key(&self) : string {
        return owned(unsafe(raw::chx_dom_key(handle)))
    }
}

// ── Document-level helpers ──────────────────────────────────────────────────
/// The page's document.
public func document() : Document {
    return Document { handle : unsafe(raw::chx_dom_document()) }
}

public func get_element_by_id(doc : &Document, id : string_view) : Option<Element> {
    return doc.get_element_by_id(id)
}

public func query_selector(doc : &Document, selector : string_view) : Option<Element> {
    return doc.query_selector(selector)
}

// ── fetch ───────────────────────────────────────────────────────────────────
/// `fetch(url)`; `handler` receives the HTTP status (0 when the request failed, and
/// `body` is then the error message) and the response text. The body is borrowed for
/// the duration of the callback — copy it if you need to keep it.
public func fetch_text(url : string_view, handler : (status : int, body : &string_view) => void) {
    unsafe { raw::chx_dom_fetch_text(url, handler) }
}

// ── localStorage ────────────────────────────────────────────────────────────
public func storage_get(key : string_view) : Option<string> {
    var p = unsafe(raw::chx_dom_storage_get(key))
    if(p == null) {
        return Option.None<string>()
    }
    return Option.Some<string>(owned(p))
}

public func storage_set(key : string_view, value : string_view) {
    unsafe { raw::chx_dom_storage_set(key, value) }
}

public func storage_remove(key : string_view) {
    unsafe { raw::chx_dom_storage_remove(key) }
}

// ── URL ─────────────────────────────────────────────────────────────────────
public func url_origin(url : string_view) : string {
    return owned(unsafe(raw::chx_dom_url_origin(url)))
}

public func url_pathname(url : string_view) : string {
    return owned(unsafe(raw::chx_dom_url_pathname(url)))
}

public func url_search_param(url : string_view, name : string_view) : Option<string> {
    var p = unsafe(raw::chx_dom_url_search_param(url, name))
    if(p == null) {
        return Option.None<string>()
    }
    return Option.Some<string>(owned(p))
}

} // namespace dom
