// A small DOM bridge for the freestanding webassembly target.
//
// The safe surface is this file: `Node` values are opaque u32 handles into a
// table owned by the JS host (lang/runtime/wasm/host.mjs), and text/attributes
// travel as (pointer, length) scalars read out of linear memory. The raw
// @extern declarations under it never take a Chemical aggregate by value, so
// the first wasm wave does not depend on a wasm C aggregate ABI.
//
// Native targets have no implementation of the `chc_wasm_dom_*` imports; a
// native build that imports this module fails at link time. That is deliberate:
// this is the browser-side UI foundation, not a portable DOM.
//
// Exports (name them with `--wasm-export`):
//   - any app entry the user defines
//   - `chc_wasm_on_event` (this file) — the host calls it when a listener fires

using std::string_view;
using std::function;

public namespace browser {

    public comptime const EVENT_CLICK = 1u32
    public comptime const EVENT_INPUT = 2u32
    public comptime const EVENT_CHANGE = 3u32

    // ---------------------------------------------------------------------
    // raw host imports (scalar ABI only)
    // ---------------------------------------------------------------------

    @extern @no_mangle
    public unsafe func chc_wasm_dom_body() : u32

    @extern @no_mangle
    public unsafe func chc_wasm_dom_create_element(tag : *char, tag_len : size_t) : u32

    @extern @no_mangle
    public unsafe func chc_wasm_dom_set_text(node : u32, text : *char, text_len : size_t)

    @extern @no_mangle
    public unsafe func chc_wasm_dom_append_child(parent : u32, child : u32)

    @extern @no_mangle
    public unsafe func chc_wasm_dom_set_attribute(node : u32, name : *char, name_len : size_t, value : *char, value_len : size_t)

    @extern @no_mangle
    public unsafe func chc_wasm_dom_add_event_listener(node : u32, event_id : u32)

    // ---------------------------------------------------------------------
    // handles
    // ---------------------------------------------------------------------

    public struct Node {

        private var id : u32

        @constructor
        func constructor(id : u32) {
            return Node { id : id }
        }

        public func is_null(&self) : bool {
            return self.id == 0
        }

        // The opaque host handle. Only the bridge needs it; it is stable for the
        // lifetime of the page and never aliases a different node.
        public func handle(&self) : u32 {
            return self.id
        }
    }

    // ---------------------------------------------------------------------
    // safe surface
    // ---------------------------------------------------------------------

    public func document_body() : Node {
        return Node(unsafe(chc_wasm_dom_body()))
    }

    public func create_element(tag : string_view) : Node {
        return Node(unsafe(chc_wasm_dom_create_element(tag.data(), tag.size())))
    }

    public func set_text(node : Node, text : string_view) {
        unsafe { chc_wasm_dom_set_text(node.handle(), text.data(), text.size()) }
    }

    public func append_child(parent : Node, child : Node) {
        unsafe { chc_wasm_dom_append_child(parent.handle(), child.handle()) }
    }

    public func set_attribute(node : Node, name : string_view, value : string_view) {
        unsafe {
            chc_wasm_dom_set_attribute(node.handle(), name.data(), name.size(), value.data(), value.size())
        }
    }

    public func add_event_listener(node : Node, event_id : u32, handler : std::function<() => void>) {
        unsafe { push_entry(node.handle(), event_id, handler) }
        unsafe { chc_wasm_dom_add_event_listener(node.handle(), event_id) }
    }

    public func on_click(node : Node, handler : std::function<() => void>) {
        add_event_listener(node, EVENT_CLICK, handler)
    }

    // ---------------------------------------------------------------------
    // event dispatch table
    //
    // A singly linked list of heap entries, allocated lazily and never freed
    // (the module's lifetime is the page's lifetime). Global access is wrapped
    // in `unsafe`; the safe API above is the only way to add an entry.
    // ---------------------------------------------------------------------

    struct EventEntry {
        var node : u32
        var event_id : u32
        var handler : std::function<() => void>
        var next : *mut EventEntry
    }

    var event_head : *mut EventEntry = null

    unsafe func push_entry(node : u32, event_id : u32, handler : std::function<() => void>) {
        unsafe {
            var entry = new EventEntry {
                node : node,
                event_id : event_id,
                handler : handler,
                next : event_head
            }
            event_head = entry
        }
    }

    @no_mangle
    public func chc_wasm_on_event(node : u32, event_id : u32) {
        var cur = unsafe(event_head)
        while(cur != null) {
            unsafe {
                if(cur.node == node && cur.event_id == event_id) {
                    cur.handler()
                }
                cur = cur.next
            }
        }
    }
}
