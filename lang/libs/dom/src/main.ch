// Typed DOM access for client modules (proposal 0008, minimal v1 slice).
//
// `dom` is the checked alternative to `#js` for browser code: `document()`,
// `query_selector`, `create_element`, events, `fetch`, `localStorage` and `URL`,
// with ordinary Chemical types. A typo'd method is a compile error, and the JS
// backend maps each call to the browser API (see `raw` below).
//
// Client modules only. The handles name browser objects, so the server job (native)
// has no implementation for them: import `dom` from a `client` module, never from
// `shared` or `server`. A native build that reaches one of these symbols is refused
// by the shared per-symbol target table (`MIRTargetSymbolRule` in
// `compiler/mir/MIRTargetSupport.cpp`), so the mistake is a translation-time
// diagnostic, not a link error.
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
// the symbol name to `rt.dom.*`; the C/LLVM backends have no definition for these and
// refuse to emit a call to them (the `chx_dom_` rule in `MIRTargetSupport`), which is
// why client modules own them. A handle is `0` for "no element", so an `Option` is built on the safe side
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
    func chx_dom_has_class(handle : usize, name : string_view) : int

    @extern @no_mangle
    func chx_dom_add_class(handle : usize, name : string_view)

    @extern @no_mangle
    func chx_dom_remove_class(handle : usize, name : string_view)

    @extern @no_mangle
    func chx_dom_toggle_class(handle : usize, name : string_view) : int

    @extern @no_mangle
    func chx_dom_parent(handle : usize) : usize

    @extern @no_mangle
    func chx_dom_child_count(handle : usize) : int

    @extern @no_mangle
    func chx_dom_child_at(handle : usize, index : int) : usize

    @extern @no_mangle
    func chx_dom_matches(handle : usize, selector : string_view) : int

    @extern @no_mangle
    func chx_dom_closest(handle : usize, selector : string_view) : usize

    // ── Canvas 2D ───────────────────────────────────────────────────────────
    // A context is a handle like an element: `0` means the element is not a
    // canvas (or a 2D context is unavailable), so the safe side returns an
    // `Option`. Coordinates are `f64` — the CSS pixel space the canvas API uses.
    @extern @no_mangle
    func chx_dom_get_context_2d(handle : usize) : usize

    @extern @no_mangle
    func chx_dom_canvas_fill_rect(ctx : usize, x : f64, y : f64, w : f64, h : f64)

    @extern @no_mangle
    func chx_dom_canvas_clear_rect(ctx : usize, x : f64, y : f64, w : f64, h : f64)

    @extern @no_mangle
    func chx_dom_canvas_begin_path(ctx : usize)

    @extern @no_mangle
    func chx_dom_canvas_move_to(ctx : usize, x : f64, y : f64)

    @extern @no_mangle
    func chx_dom_canvas_line_to(ctx : usize, x : f64, y : f64)

    @extern @no_mangle
    func chx_dom_canvas_arc(ctx : usize, x : f64, y : f64, radius : f64, start : f64, end : f64)

    @extern @no_mangle
    func chx_dom_canvas_close_path(ctx : usize)

    @extern @no_mangle
    func chx_dom_canvas_fill(ctx : usize)

    @extern @no_mangle
    func chx_dom_canvas_stroke(ctx : usize)

    @extern @no_mangle
    func chx_dom_canvas_save(ctx : usize)

    @extern @no_mangle
    func chx_dom_canvas_restore(ctx : usize)

    @extern @no_mangle
    func chx_dom_canvas_set_fill_style(ctx : usize, value : string_view)

    @extern @no_mangle
    func chx_dom_canvas_set_stroke_style(ctx : usize, value : string_view)

    @extern @no_mangle
    func chx_dom_canvas_set_line_width(ctx : usize, width : f64)

    @extern @no_mangle
    func chx_dom_canvas_set_font(ctx : usize, font : string_view)

    @extern @no_mangle
    func chx_dom_canvas_fill_text(ctx : usize, text : string_view, x : f64, y : f64)

    @extern @no_mangle
    func chx_dom_canvas_translate(ctx : usize, x : f64, y : f64)

    @extern @no_mangle
    func chx_dom_canvas_rotate(ctx : usize, radians : f64)

    @extern @no_mangle
    func chx_dom_canvas_scale(ctx : usize, x : f64, y : f64)

    @extern @no_mangle
    func chx_dom_canvas_set_transform(ctx : usize, a : f64, b : f64, c : f64, d : f64, e : f64, f : f64)

    // ── Animation frames and pointer events ─────────────────────────────────
    @extern @no_mangle
    func chx_dom_request_animation_frame(callback : (time_ms : f64) => void) : int

    @extern @no_mangle
    func chx_dom_event_client_x(event : usize) : f64

    @extern @no_mangle
    func chx_dom_event_client_y(event : usize) : f64

    @extern @no_mangle
    func chx_dom_event_button(event : usize) : int

    @extern @no_mangle
    func chx_dom_event_pointer_id(event : usize) : int

    @extern @no_mangle
    func chx_dom_cancel_animation_frame(id : int)

    @extern @no_mangle
    func chx_dom_set_pointer_capture(handle : usize, pointer_id : int)

    @extern @no_mangle
    func chx_dom_release_pointer_capture(handle : usize, pointer_id : int)

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
    func chx_dom_fetch_status(url : string_view) : core::async::FutureHandle<'static, int>

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

    // ── Classes ─────────────────────────────────────────────────────────────
    // The `class` attribute as a token set, so a class name is never spliced into a
    // string: `toggle_class` is what a `class="…"` read-modify-write would break
    // whenever two pieces of code touch the same element.
    public func has_class(&self, name : string_view) : bool {
        return unsafe(raw::chx_dom_has_class(handle, name)) != 0
    }

    public func add_class(&self, name : string_view) {
        unsafe { raw::chx_dom_add_class(handle, name) }
    }

    public func remove_class(&self, name : string_view) {
        unsafe { raw::chx_dom_remove_class(handle, name) }
    }

    /// Adds `name` when absent, removes it when present; returns whether it is now set.
    public func toggle_class(&self, name : string_view) : bool {
        return unsafe(raw::chx_dom_toggle_class(handle, name)) != 0
    }

    // ── Traversal ───────────────────────────────────────────────────────────
    /// The parent element (`None` for the document element or a detached node).
    public func parent(&self) : Option<Element> {
        var h = unsafe(raw::chx_dom_parent(handle))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// Number of *element* children (text nodes are not counted, matching `children`).
    public func child_count(&self) : int {
        return unsafe(raw::chx_dom_child_count(handle))
    }

    /// The element child at `index`, or `None` when there is none.
    public func child_at(&self, index : int) : Option<Element> {
        var h = unsafe(raw::chx_dom_child_at(handle, index))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// Whether this element matches a CSS selector (the element itself, not its
    /// descendants — `query_selector` is the descendant search).
    public func matches(&self, selector : string_view) : bool {
        return unsafe(raw::chx_dom_matches(handle, selector)) != 0
    }

    /// The nearest ancestor-or-self matching `selector`.
    public func closest(&self, selector : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_closest(handle, selector))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// The element's 2D drawing context — `None` when this element is not a
    /// `<canvas>` (or the host has no 2D context for it).
    public func get_context_2d(&self) : Option<Canvas2D> {
        var h = unsafe(raw::chx_dom_get_context_2d(handle))
        if(h == 0) {
            return Option.None<Canvas2D>()
        }
        return Option.Some<Canvas2D>(Canvas2D { handle : h })
    }

    /// Capture pointer events for `pointer_id` until `release_pointer_capture`
    /// (a drag that keeps receiving moves when the pointer leaves the element).
    public func set_pointer_capture(&self, pointer_id : int) {
        unsafe { raw::chx_dom_set_pointer_capture(handle, pointer_id) }
    }

    public func release_pointer_capture(&self, pointer_id : int) {
        unsafe { raw::chx_dom_release_pointer_capture(handle, pointer_id) }
    }
}

// ── Canvas 2D ───────────────────────────────────────────────────────────────
/// A `<canvas>` 2D context, as a host handle.
///
/// The drawing surface an app needs instead of `#js`: paths (`begin_path`,
/// `move_to`, `line_to`, `arc`), state (`save`/`restore`, styles, line width),
/// transforms and `fill_text`. Coordinates are CSS pixels, matching the browser
/// API, so a wrapper written over `#js` keeps its arithmetic.
@direct_init
public struct Canvas2D {
    internal var handle : usize

    public func fill_rect(&self, x : f64, y : f64, w : f64, h : f64) {
        unsafe { raw::chx_dom_canvas_fill_rect(handle, x, y, w, h) }
    }

    public func clear_rect(&self, x : f64, y : f64, w : f64, h : f64) {
        unsafe { raw::chx_dom_canvas_clear_rect(handle, x, y, w, h) }
    }

    public func begin_path(&self) {
        unsafe { raw::chx_dom_canvas_begin_path(handle) }
    }

    public func move_to(&self, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_move_to(handle, x, y) }
    }

    public func line_to(&self, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_line_to(handle, x, y) }
    }

    public func arc(&self, x : f64, y : f64, radius : f64, start : f64, end : f64) {
        unsafe { raw::chx_dom_canvas_arc(handle, x, y, radius, start, end) }
    }

    public func close_path(&self) {
        unsafe { raw::chx_dom_canvas_close_path(handle) }
    }

    public func fill(&self) {
        unsafe { raw::chx_dom_canvas_fill(handle) }
    }

    public func stroke(&self) {
        unsafe { raw::chx_dom_canvas_stroke(handle) }
    }

    /// Push/pop the whole context state (style, transform, line width).
    public func save(&self) {
        unsafe { raw::chx_dom_canvas_save(handle) }
    }

    public func restore(&self) {
        unsafe { raw::chx_dom_canvas_restore(handle) }
    }

    /// A CSS color (`"#0af"`, `"rgb(0, 170, 255)"`).
    public func set_fill_style(&self, value : string_view) {
        unsafe { raw::chx_dom_canvas_set_fill_style(handle, value) }
    }

    public func set_stroke_style(&self, value : string_view) {
        unsafe { raw::chx_dom_canvas_set_stroke_style(handle, value) }
    }

    public func set_line_width(&self, width : f64) {
        unsafe { raw::chx_dom_canvas_set_line_width(handle, width) }
    }

    /// A CSS font shorthand (`"16px sans-serif"`); used by `fill_text`.
    public func set_font(&self, font : string_view) {
        unsafe { raw::chx_dom_canvas_set_font(handle, font) }
    }

    public func fill_text(&self, text : string_view, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_fill_text(handle, text, x, y) }
    }

    public func translate(&self, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_translate(handle, x, y) }
    }

    public func rotate(&self, radians : f64) {
        unsafe { raw::chx_dom_canvas_rotate(handle, radians) }
    }

    public func scale(&self, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_scale(handle, x, y) }
    }

    /// Replace the transform with `[a b c d e f]` (the canvas matrix order).
    public func set_transform(&self, a : f64, b : f64, c : f64, d : f64, e : f64, f : f64) {
        unsafe { raw::chx_dom_canvas_set_transform(handle, a, b, c, d, e, f) }
    }
}

/// Run `callback` before the next paint, with the frame's timestamp in
/// milliseconds. Returns a cancellation id for `cancel_animation_frame`.
public func request_animation_frame(callback : (time_ms : f64) => void) : int {
    return unsafe(raw::chx_dom_request_animation_frame(callback))
}

public func cancel_animation_frame(id : int) {
    unsafe { raw::chx_dom_cancel_animation_frame(id) }
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

    /// Pointer position in CSS pixels, relative to the viewport.
    public func client_x(&self) : f64 {
        return unsafe(raw::chx_dom_event_client_x(handle))
    }

    public func client_y(&self) : f64 {
        return unsafe(raw::chx_dom_event_client_y(handle))
    }

    /// The mouse button (`0` primary, `1` middle, `2` secondary; `-1` for no button).
    public func button(&self) : int {
        return unsafe(raw::chx_dom_event_button(handle))
    }

    /// The pointer that produced the event (`-1` for a non-pointer event).
    public func pointer_id(&self) : int {
        return unsafe(raw::chx_dom_event_pointer_id(handle))
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

/// `fetch(url)` in awaitable form: the future resolves to the HTTP status, and `0`
/// means the request failed. Await it inside an `async func`:
///
/// ```
/// async func load(url : string_view) : int {
///     var status = await dom::fetch_status(url)
///     return status
/// }
/// ```
///
/// A client-only entry point: the native runtimes have no DOM, so this resolves only
/// where the host supplies it (a browser, or the JS backend's harness).
public func fetch_status(url : string_view) : core::async::FutureHandle<'static, int> {
    return unsafe(raw::chx_dom_fetch_status(url))
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
