// Typed DOM access for client modules (proposal 0008, minimal v1 slice).
//
// `dom` is the checked alternative to `#js` for browser code: `document()`,
// `querySelector`, `createElement`, events, `fetch`, `localStorage` and `URL`,
// with ordinary Chemical types. The Chemical surface uses the browser's own
// spelling (`querySelector`, `activeElement`, `preventDefault`, …), so JS code
// and component bodies transfer without a mapping table. A typo'd method is a
// compile error, and the JS backend maps each call to the browser API (see
// `raw` below).
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
//     var button = doc.querySelector("#count").expect("button")
//     button.addEventListener("click", onClick)
//
//     public func onClick(e : &dom::Event) {
//         e.preventDefault()
//         var label = dom::document().getElementById("count").expect("label")
//         label.setText("1")
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
    func chx_dom_window() : usize

    @extern @no_mangle
    func chx_dom_navigator() : usize

    @extern @no_mangle
    func chx_dom_body(doc : usize) : usize

    @extern @no_mangle
    func chx_dom_active_element(doc : usize) : usize

    @extern @no_mangle
    func chx_dom_title(doc : usize) : *char

    @extern @no_mangle
    func chx_dom_document_element(doc : usize) : usize

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

    // ── Timers, animation frames and pointer events ─────────────────────────
    @extern @no_mangle
    func chx_dom_set_timeout(callback : () => void, delay_ms : int) : int

    @extern @no_mangle
    func chx_dom_clear_timeout(id : int)

    @extern @no_mangle
    func chx_dom_request_animation_frame(callback : (time_ms : f64) => void) : int

    @extern @no_mangle
    func chx_dom_cancel_animation_frame(id : int)

    @extern @no_mangle
    func chx_dom_event_client_x(event : usize) : f64

    @extern @no_mangle
    func chx_dom_event_client_y(event : usize) : f64

    @extern @no_mangle
    func chx_dom_event_button(event : usize) : int

    @extern @no_mangle
    func chx_dom_event_pointer_id(event : usize) : int

    @extern @no_mangle
    func chx_dom_set_pointer_capture(handle : usize, pointer_id : int)

    @extern @no_mangle
    func chx_dom_release_pointer_capture(handle : usize, pointer_id : int)

    @extern @no_mangle
    func chx_dom_on(handle : usize, event : string_view, handler : (e : &Event) => void)

    @extern @no_mangle
    func chx_dom_off(handle : usize, event : string_view, handler : (e : &Event) => void)

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
    func chx_dom_navigator_clipboard(navigator : usize) : usize

    @extern @no_mangle
    func chx_dom_clipboard_write_text(clipboard : usize, text : string_view)

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
    public func getElementById(&self, id : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_get_element_by_id(handle, id))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// The first element matching a CSS selector.
    public func querySelector(&self, selector : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_query_selector(handle, selector))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    public func createElement(&self, tag : string_view) : Element {
        return Element { handle : unsafe(raw::chx_dom_create_element(handle, tag)) }
    }

    public func body(&self) : Element {
        return Element { handle : unsafe(raw::chx_dom_body(handle)) }
    }

    /// The element that currently has keyboard focus, if any.
    public func activeElement(&self) : Option<Element> {
        var h = unsafe(raw::chx_dom_active_element(handle))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    public func title(&self) : string {
        return owned(unsafe(raw::chx_dom_title(handle)))
    }

    /// The root `<html>` element.
    public func documentElement(&self) : Element {
        return Element { handle : unsafe(raw::chx_dom_document_element(handle)) }
    }

    public func addEventListener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_on(handle, event, handler) }
    }

    public func removeEventListener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_off(handle, event, handler) }
    }
}

// ── Element ─────────────────────────────────────────────────────────────────
@direct_init
public struct Element {
    internal var handle : usize

    public func setText(&self, text : string_view) {
        unsafe { raw::chx_dom_set_text(handle, text) }
    }

    public func textContent(&self) : string {
        return owned(unsafe(raw::chx_dom_text(handle)))
    }

    /// Trusted markup only: the string is parsed as HTML (never pass user input).
    public func setHTML(&self, html : string_view) {
        unsafe { raw::chx_dom_set_html(handle, html) }
    }

    public func setAttribute(&self, name : string_view, value : string_view) {
        unsafe { raw::chx_dom_set_attribute(handle, name, value) }
    }

    public func getAttribute(&self, name : string_view) : Option<string> {
        var p = unsafe(raw::chx_dom_get_attribute(handle, name))
        if(p == null) {
            return Option.None<string>()
        }
        return Option.Some<string>(owned(p))
    }

    public func hasAttribute(&self, name : string_view) : bool {
        return unsafe(raw::chx_dom_has_attribute(handle, name)) != 0
    }

    public func removeAttribute(&self, name : string_view) {
        unsafe { raw::chx_dom_remove_attribute(handle, name) }
    }

    /// An `<input>`/`<textarea>` value.
    public func setValue(&self, value : string_view) {
        unsafe { raw::chx_dom_set_value(handle, value) }
    }

    public func value(&self) : string {
        return owned(unsafe(raw::chx_dom_value(handle)))
    }

    public func appendChild(&self, child : &Element) {
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
    public func addEventListener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_on(handle, event, handler) }
    }

    public func removeEventListener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_off(handle, event, handler) }
    }

    public func querySelector(&self, selector : string_view) : Option<Element> {
        var h = unsafe(raw::chx_dom_query_selector(handle, selector))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    // ── Classes ─────────────────────────────────────────────────────────────
    // The `class` attribute as a token set, so a class name is never spliced into a
    // string: `toggleClass` is what a `class="…"` read-modify-write would break
    // whenever two pieces of code touch the same element.
    public func hasClass(&self, name : string_view) : bool {
        return unsafe(raw::chx_dom_has_class(handle, name)) != 0
    }

    public func addClass(&self, name : string_view) {
        unsafe { raw::chx_dom_add_class(handle, name) }
    }

    public func removeClass(&self, name : string_view) {
        unsafe { raw::chx_dom_remove_class(handle, name) }
    }

    /// Adds `name` when absent, removes it when present; returns whether it is now set.
    public func toggleClass(&self, name : string_view) : bool {
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
    public func childCount(&self) : int {
        return unsafe(raw::chx_dom_child_count(handle))
    }

    /// The element child at `index`, or `None` when there is none.
    public func childAt(&self, index : int) : Option<Element> {
        var h = unsafe(raw::chx_dom_child_at(handle, index))
        if(h == 0) {
            return Option.None<Element>()
        }
        return Option.Some<Element>(Element { handle : h })
    }

    /// Whether this element matches a CSS selector (the element itself, not its
    /// descendants — `querySelector` is the descendant search).
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
    public func getContext2D(&self) : Option<Canvas2D> {
        var h = unsafe(raw::chx_dom_get_context_2d(handle))
        if(h == 0) {
            return Option.None<Canvas2D>()
        }
        return Option.Some<Canvas2D>(Canvas2D { handle : h })
    }

    /// Capture pointer events for `pointerId` until `releasePointerCapture`
    /// (a drag that keeps receiving moves when the pointer leaves the element).
    public func setPointerCapture(&self, pointer_id : int) {
        unsafe { raw::chx_dom_set_pointer_capture(handle, pointer_id) }
    }

    public func releasePointerCapture(&self, pointer_id : int) {
        unsafe { raw::chx_dom_release_pointer_capture(handle, pointer_id) }
    }
}

// ── Canvas 2D ───────────────────────────────────────────────────────────────
/// A `<canvas>` 2D context, as a host handle.
///
/// The drawing surface an app needs instead of `#js`: paths (`beginPath`,
/// `moveTo`, `lineTo`, `arc`), state (`save`/`restore`, styles, line width),
/// transforms and `fillText`. Coordinates are CSS pixels, matching the browser
/// API, so a wrapper written over `#js` keeps its arithmetic.
@direct_init
public struct Canvas2D {
    internal var handle : usize

    public func fillRect(&self, x : f64, y : f64, w : f64, h : f64) {
        unsafe { raw::chx_dom_canvas_fill_rect(handle, x, y, w, h) }
    }

    public func clearRect(&self, x : f64, y : f64, w : f64, h : f64) {
        unsafe { raw::chx_dom_canvas_clear_rect(handle, x, y, w, h) }
    }

    public func beginPath(&self) {
        unsafe { raw::chx_dom_canvas_begin_path(handle) }
    }

    public func moveTo(&self, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_move_to(handle, x, y) }
    }

    public func lineTo(&self, x : f64, y : f64) {
        unsafe { raw::chx_dom_canvas_line_to(handle, x, y) }
    }

    public func arc(&self, x : f64, y : f64, radius : f64, start : f64, end : f64) {
        unsafe { raw::chx_dom_canvas_arc(handle, x, y, radius, start, end) }
    }

    public func closePath(&self) {
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
    public func setFillStyle(&self, value : string_view) {
        unsafe { raw::chx_dom_canvas_set_fill_style(handle, value) }
    }

    public func setStrokeStyle(&self, value : string_view) {
        unsafe { raw::chx_dom_canvas_set_stroke_style(handle, value) }
    }

    public func setLineWidth(&self, width : f64) {
        unsafe { raw::chx_dom_canvas_set_line_width(handle, width) }
    }

    /// A CSS font shorthand (`"16px sans-serif"`); used by `fillText`.
    public func setFont(&self, font : string_view) {
        unsafe { raw::chx_dom_canvas_set_font(handle, font) }
    }

    public func fillText(&self, text : string_view, x : f64, y : f64) {
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
    public func setTransform(&self, a : f64, b : f64, c : f64, d : f64, e : f64, f : f64) {
        unsafe { raw::chx_dom_canvas_set_transform(handle, a, b, c, d, e, f) }
    }
}

// ── Event ───────────────────────────────────────────────────────────────────
@direct_init
public struct Event {
    internal var handle : usize

    public func preventDefault(&self) {
        unsafe { raw::chx_dom_prevent_default(handle) }
    }

    public func stopPropagation(&self) {
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
    public func clientX(&self) : f64 {
        return unsafe(raw::chx_dom_event_client_x(handle))
    }

    public func clientY(&self) : f64 {
        return unsafe(raw::chx_dom_event_client_y(handle))
    }

    /// The mouse button (`0` primary, `1` middle, `2` secondary; `-1` for no button).
    public func button(&self) : int {
        return unsafe(raw::chx_dom_event_button(handle))
    }

    /// The pointer that produced the event (`-1` for a non-pointer event).
    public func pointerId(&self) : int {
        return unsafe(raw::chx_dom_event_pointer_id(handle))
    }
}

// ── Window ──────────────────────────────────────────────────────────────────
/// The browser window: timers, animation frames and window-level events.
@direct_init
public struct Window {
    internal var handle : usize

    /// Run `callback` after `delay_ms`. Returns an id for `clearTimeout`.
    public func setTimeout(&self, callback : () => void, delay_ms : int) : int {
        return unsafe(raw::chx_dom_set_timeout(callback, delay_ms))
    }

    public func clearTimeout(&self, id : int) {
        unsafe { raw::chx_dom_clear_timeout(id) }
    }

    /// Run `callback` before the next paint, with the frame's timestamp in
    /// milliseconds. Returns a cancellation id for `cancelAnimationFrame`.
    public func requestAnimationFrame(&self, callback : (time_ms : f64) => void) : int {
        return unsafe(raw::chx_dom_request_animation_frame(callback))
    }

    public func cancelAnimationFrame(&self, id : int) {
        unsafe { raw::chx_dom_cancel_animation_frame(id) }
    }

    public func addEventListener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_on(handle, event, handler) }
    }

    public func removeEventListener(&self, event : string_view, handler : (e : &Event) => void) {
        unsafe { raw::chx_dom_off(handle, event, handler) }
    }
}

// ── Storage ─────────────────────────────────────────────────────────────────
/// `localStorage`: a string key/value store owned by the browser.
@direct_init
public struct Storage {
    internal var handle : usize

    public func getItem(&self, key : string_view) : Option<string> {
        var p = unsafe(raw::chx_dom_storage_get(key))
        if(p == null) {
            return Option.None<string>()
        }
        return Option.Some<string>(owned(p))
    }

    public func setItem(&self, key : string_view, value : string_view) {
        unsafe { raw::chx_dom_storage_set(key, value) }
    }

    public func removeItem(&self, key : string_view) {
        unsafe { raw::chx_dom_storage_remove(key) }
    }
}

// ── Navigator / Clipboard ───────────────────────────────────────────────────
@direct_init
public struct Navigator {
    internal var handle : usize

    public func clipboard(&self) : Clipboard {
        return Clipboard { handle : unsafe(raw::chx_dom_navigator_clipboard(handle)) }
    }
}

@direct_init
public struct Clipboard {
    internal var handle : usize

    /// Copy `text` to the system clipboard.
    public func writeText(&self, text : string_view) {
        unsafe { raw::chx_dom_clipboard_write_text(handle, text) }
    }
}

// ── Browser roots ───────────────────────────────────────────────────────────
/// The page's document.
public func document() : Document {
    return Document { handle : unsafe(raw::chx_dom_document()) }
}

/// The browser window.
public func window() : Window {
    return Window { handle : unsafe(raw::chx_dom_window()) }
}

/// `localStorage`.
public func localStorage() : Storage {
    return Storage { handle : 0 }
}

/// `navigator`.
public func navigator() : Navigator {
    return Navigator { handle : unsafe(raw::chx_dom_navigator()) }
}

// ── fetch ───────────────────────────────────────────────────────────────────
/// `fetch(url)`; `handler` receives the HTTP status (0 when the request failed, and
/// `body` is then the error message) and the response text. The body is borrowed for
/// the duration of the callback — copy it if you need to keep it.
public func fetch(url : string_view, handler : (status : int, body : &string_view) => void) {
    unsafe { raw::chx_dom_fetch_text(url, handler) }
}

/// `fetch(url)` in awaitable form: the future resolves to the HTTP status, and `0`
/// means the request failed. Await it inside an `async func`:
///
/// ```
/// async func load(url : string_view) : int {
///     var status = await dom::fetchStatus(url)
///     return status
/// }
/// ```
///
/// A client-only entry point: the native runtimes have no DOM, so this resolves only
/// where the host supplies it (a browser, or the JS backend's harness).
public func fetchStatus(url : string_view) : core::async::FutureHandle<'static, int> {
    return unsafe(raw::chx_dom_fetch_status(url))
}

// ── Animation frames ────────────────────────────────────────────────────────
/// Run `callback` before the next paint, with the frame's timestamp in
/// milliseconds. Returns a cancellation id for `cancelAnimationFrame`.
public func requestAnimationFrame(callback : (time_ms : f64) => void) : int {
    return unsafe(raw::chx_dom_request_animation_frame(callback))
}

public func cancelAnimationFrame(id : int) {
    unsafe { raw::chx_dom_cancel_animation_frame(id) }
}

// ── URL ─────────────────────────────────────────────────────────────────────
public func urlOrigin(url : string_view) : string {
    return owned(unsafe(raw::chx_dom_url_origin(url)))
}

public func urlPathname(url : string_view) : string {
    return owned(unsafe(raw::chx_dom_url_pathname(url)))
}

public func urlSearchParam(url : string_view, name : string_view) : Option<string> {
    var p = unsafe(raw::chx_dom_url_search_param(url, name))
    if(p == null) {
        return Option.None<string>()
    }
    return Option.Some<string>(owned(p))
}

} // namespace dom
