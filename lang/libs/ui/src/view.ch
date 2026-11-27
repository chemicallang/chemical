// P1.3 ViewBuilder and frame-local view nodes. No layout/paint/reconcile.
using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;
using ui_internal::NodeArena;
using ui_internal::FrameArena;
using ui_internal::KeySet;

const VIEW_NONE : u32 = 0xFFFFFFFFu32

public namespace ui_internal {

public enum ViewKind {
    Root, Row, Column, Stack, Wrap, Grid, Scroll, Canvas, Fragment, Overlay,
    Component, Slot, Text, RichText, Image, Icon, VirtualList
}

@direct_init
public struct ViewNode {
    var id : NodeId
    var key : Key
    var kind : ViewKind
    var parent : Option<NodeId>
    var first_child : u32
    var last_child : u32
    var next_sibling : u32
    var child_count : u32
    var style : Style
    var state_style : Style
    var has_state_style : bool
    var text : string_view
    var span_start : u32
    var span_count : u32
    var image : ImageSource
    var icon : IconId
    var canvas : CanvasId
    var overlay_layer : OverlayLayer
    var overlay_anchor : Option<NodeId>
    var component_root : NodeId
    var slot_name : string_view
    var test_id : string_view
    var focusable : bool
    var tab_index : i32
    var command : CommandId
    var has_command : bool
    var grid_col : u16
    var grid_row : u16
    var grid_col_span : u16
    var grid_row_span : u16
    var has_grid_cell : bool
    var transform : Transform2D
    var has_transform : bool
    var anim_prop : AnimProperty
    var anim_to : float
    var anim_spec : AnimationSpec
    var has_anim : bool
    var anim_exit : bool
    var error : UiError
    var has_error : bool
    var semantics : Semantics
    var has_semantics : bool
    var virtual_count : u32
    var virtual_axis : Axis
    var scroll_anchor : ScrollAnchor
    var has_virtual : bool
}

func empty_semantics() : Semantics {
    return Semantics {
        role : Role.Unknown, label : "", description : "", value : "", alt : "",
        lang : "", placeholder : "", error : "",
        states : zeroed<SemState>(), actions : zeroed<SemAction>(),
        level : 0u8, pos_in_set : 0u32, set_size : 0u32, row_index : 0u32,
        column_index : 0u32, selected_index : 0i32,
        labelled_by : Option.None<NodeId>(), described_by : Option.None<NodeId>(),
        controls : Option.None<NodeId>(), owns : Option.None<NodeId>(),
        active_descendant : Option.None<NodeId>(), has_popup : false, secure : false
    }
}

func empty_spec() : AnimationSpec {
    return AnimationSpec {
        duration : Duration { ms : 0.0f },
        easing : Easing.EaseOut,
        delay : Duration { ms : 0.0f },
        repeat : 0u32,
        alternate : false
    }
}

func make_node(id : NodeId, kind : ViewKind, k : Key) : ViewNode {
    return ViewNode {
        id : id, key : k, kind : kind, parent : Option.None<NodeId>(),
        first_child : VIEW_NONE, last_child : VIEW_NONE, next_sibling : VIEW_NONE,
        child_count : 0u32, style : Style.new(), state_style : Style.new(),
        has_state_style : false, text : "", span_start : 0u32, span_count : 0u32,
        image : ImageSource.Placeholder(), icon : IconId.zero(), canvas : CanvasId.zero(),
        overlay_layer : OverlayLayer.Popover,
        overlay_anchor : Option.None<NodeId>(), component_root : zeroed<NodeId>(),
        slot_name : "", test_id : "", focusable : false, tab_index : 0i32,
        command : CommandId.zero(), has_command : false, grid_col : 0u16,
        grid_row : 0u16, grid_col_span : 1u16, grid_row_span : 1u16,
        has_grid_cell : false, transform : Transform2D.identity(), has_transform : false,
        anim_prop : AnimProperty.Opacity, anim_to : 0.0f, anim_spec : empty_spec(),
        has_anim : false, anim_exit : false,
        error : UiError.make(ErrorCode.Internal, ""), has_error : false,
        semantics : empty_semantics(), has_semantics : false,
        virtual_count : 0u32, virtual_axis : Axis.Vertical,
        scroll_anchor : ScrollAnchor.Start(), has_virtual : false
    }
}

} // namespace ui_internal

public namespace ui_internal {
@direct_init
public struct ScopeStack {
    var keys : [64]Key
    var count : u32

    func push(&mut self, k : Key) {
        if(self.count < 64u32) { self.keys[self.count as size_t] = k; self.count = self.count + 1u32 }
    }
    func pop(&mut self) {
        if(self.count > 0u32) { self.count = self.count - 1u32 }
    }
    func current(&self) : Option<Key> {
        if(self.count == 0u32) { return Option.None<Key>() }
        return Option.Some<Key>(self.keys[(self.count - 1u32) as size_t])
    }
}
}

public struct ScopeGuard {
    internal var owner : *mut ui_internal::ScopeStack
    internal var active : bool

    @delete func delete(&mut self) {
        if(self.active) {
            unsafe { (*self.owner).pop() }
        }
    }
}

public struct ViewBuilder {
    var arena : NodeArena
    var frame_storage : FrameArena
    var nodes : vector<ui_internal::ViewNode>
    var spans : vector<TextSpan>
    var scope_state : ui_internal::ScopeStack
    var keys_seen : KeySet
    var root : NodeId
    var frame_data : FrameInfo
    var viewport_data : Viewport
    var node_count_value : u32
    var diagnostic_count : u32
    var diagnostic_code : ErrorCode

    func make_internal(capacity : u32, width : float, height : float) : ViewBuilder {
        var viewport = Viewport {
            size : Size.new(width, height),
            safe_area : Insets.zero(),
            breakpoint : Breakpoint.Lg
        }
        var frame = FrameInfo {
            window : zeroed<WindowId>(), viewport : viewport, scale : 1.0f,
            text_scale : 1.0f, theme : ThemeMode.Light, reduced_motion : false,
            locale : "en-US", direction : Direction.Ltr, frame : 0u64,
            now_ms : 0.0, focused : true, occluded : false
        }
        var b = ViewBuilder {
            arena : NodeArena.new(capacity),
            frame_storage : FrameArena.new(16u * 1024u * 1024u),
            nodes : vector<ui_internal::ViewNode>(),
            spans : vector<TextSpan>(),
            scope_state : ui_internal::ScopeStack { keys : zeroed<[64]Key>(), count : 0u32 },
            keys_seen : KeySet.new(capacity + 1u32),
            root : zeroed<NodeId>(),
            frame_data : frame,
            viewport_data : viewport,
            node_count_value : 0u32,
            diagnostic_count : 0u32,
            diagnostic_code : ErrorCode.Other
        }
        b.nodes.reserve(capacity as size_t)
        b.spans.reserve(capacity as size_t)
        var r = b.add_node(ui_internal::ViewKind.Root, key("root"))
        b.root = r
        return b
    }

    func record(&mut self, code : ErrorCode) {
        self.diagnostic_count = self.diagnostic_count + 1u32
        self.diagnostic_code = code
    }

    func node_ptr(&mut self, id : NodeId) : *mut ui_internal::ViewNode {
        if(!self.arena.contains(id)) { return null }
        var ix = (id.index() - 1u32) as size_t
        if(ix >= self.nodes.size()) { return null }
        return self.nodes.get_ptr(ix)
    }

    func add_node(&mut self, kind : ui_internal::ViewKind, k : Key) : NodeId {
        var r = self.arena.insert()
        if(r is Result.Err) { self.record(ErrorCode.ResourceExhausted); return zeroed<NodeId>() }
        var Ok(id) = r else return zeroed<NodeId>()
        self.nodes.push(ui_internal::make_node(id, kind, k))
        self.node_count_value = self.node_count_value + 1u32
        var kr = self.keys_seen.insert(k)
        if(kr == false) { self.record(ErrorCode.DuplicateKey) }
        return id
    }

    func copy_text(&mut self, value : string_view) : string_view {
        if(value.size() == 0u) { return "" }
        var p = self.frame_storage.alloc(value.size(), 1u)
        if(p is Result.Err) { self.record(ErrorCode.OutOfMemory); return "" }
        var Ok(dst) = p else return ""
        unsafe { memcpy(dst as *mut void, value.data() as *mut void, value.size()) }
        return unsafe(std::string_view(dst as *char, value.size()))
    }

    func copy_span(&mut self, span : TextSpan) : TextSpan {
        var out = span
        out.text = self.copy_text(span.text)
        if(span.link is Option.Some) {
            var Some(link) = span.link else return out
            out.link = Option.Some<string_view>(self.copy_text(link))
        } else {
            out.link = Option.None<string_view>()
        }
        if(span.features.raw_tags.size() > 0u) {
            var n = span.features.raw_tags.size()
            var rp = self.frame_storage.alloc(n * 4u, 4u)
            if(rp is Result.Ok) {
                var Ok(ptr) = rp else return out
                unsafe { memcpy(ptr as *mut void, span.features.raw_tags.data() as *mut void, n * 4u) }
                out.features.raw_tags = unsafe(std::span<u32>(ptr as *mut u32, n))
            } else {
                self.record(ErrorCode.OutOfMemory)
            }
        }
        return out
    }

    public func frame(&self) : &FrameInfo { return &self.frame_data }
    public func viewport(&self) : Viewport { return self.viewport_data }
    public func scroll_offset(&self, k : Key) : Point { return Point.zero() }

    func container(&mut self, kind : ui_internal::ViewKind, k : Key, style : Style) : NodeId {
        var id = self.add_node(kind, k)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.style = style } }
        return id
    }

    public func row(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Row, k, style) }
    public func column(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Column, k, style) }
    public func stack(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Stack, k, style) }
    public func wrap(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Wrap, k, style) }

    public func grid(&mut self, k : Key, style : Style, tracks : GridTracks) : NodeId {
        var id = self.container(ui_internal::ViewKind.Grid, k, style)
        var p = self.node_ptr(id)
        if(p != null) {
            unsafe {
                p.has_grid_cell = false
                // Track arrays are contract-owned until P1.6; store counts only.
                p.grid_col = tracks.columns.size() as u16
                p.grid_row = tracks.rows.size() as u16
            }
        }
        return id
    }

    public func scroll(&mut self, k : Key, style : Style, axis : Axis, anchor : ScrollAnchor = ScrollAnchor.Start()) : NodeId {
        var id = self.container(ui_internal::ViewKind.Scroll, k, style)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.virtual_axis = axis; p.scroll_anchor = anchor; p.has_virtual = true } }
        return id
    }

    public func canvas(&mut self, k : Key, style : Style, canvas : CanvasId) : NodeId {
        var id = self.container(ui_internal::ViewKind.Canvas, k, style)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.canvas = canvas } }
        return id
    }

    public func fragment(&mut self, k : Key) : NodeId { return self.add_node(ui_internal::ViewKind.Fragment, k) }

    public func scope(&mut self, k : Key) : ScopeGuard {
        self.scope_state.push(k)
        return ScopeGuard { owner : unsafe(&raw mut self.scope_state), active : true }
    }
    func pop_scope(&mut self) {
        self.scope_state.pop()
    }
    public func scoped_key(&self, parent : Key, label : string_view) : Key {
        return key_pair(label, parent.id(), parent.tag())
    }

    public func overlay(&mut self, k : Key, layer : OverlayLayer, anchor : Option<NodeId>) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Overlay, k)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.overlay_layer = layer; p.overlay_anchor = anchor } }
        return id
    }

    public func component(&mut self, k : Key, name : string_view) : Component {
        var id = self.add_node(ui_internal::ViewKind.Component, k)
        var n = self.copy_text(name)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.slot_name = n; p.component_root = id } }
        return Component { root : id, key_value : k, name_value : n }
    }

    public func slot(&mut self, c : Component, name : string_view) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Slot, c.key_value)
        var n = self.copy_text(name)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.slot_name = n; p.component_root = c.root } }
        if(c.root.is_valid()) { self.child(c.root, id) }
        return id
    }

    public func text(&mut self, k : Key, value : string_view, style : Style) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Text, k)
        var p = self.node_ptr(id)
        if(p != null) {
            unsafe { p.text = self.copy_text(value); p.style = style }
        }
        return id
    }

    public func rich_text(&mut self, k : Key, spans : &[TextSpan], style : Style) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.RichText, k)
        var start = self.spans.size() as u32
        var i : size_t = 0
        while(i < spans.size()) {
            var copied = self.copy_span(spans[i])
            self.spans.push(copied)
            i = i + 1u
        }
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.span_start = start; p.span_count = self.spans.size() as u32 - start; p.style = style } }
        return id
    }

    public func image(&mut self, k : Key, src : ImageSource, style : Style) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Image, k)
        var p = self.node_ptr(id)
        if(p != null) {
            switch(src) {
                Path(path) => { unsafe { p.image = ImageSource.Path(self.copy_text(path)); p.style = style } }
                Bytes(bytes) => {
                    // P1.3a choice: bytes are not copied into the frame arena and
                    // there is no image cache yet. Reject with a typed diagnostic.
                    self.record(ErrorCode.Unsupported)
                    unsafe { p.image = ImageSource.Placeholder(); p.style = style }
                }
                Handle(h) => { unsafe { p.image = ImageSource.Handle(h); p.style = style } }
                Placeholder() => { unsafe { p.image = ImageSource.Placeholder(); p.style = style } }
            }
        }
        return id
    }

    public func icon(&mut self, k : Key, icon : IconId, style : Style) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Icon, k)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.icon = icon; p.style = style } }
        return id
    }

    public func <S : ItemSource> virtual_list(&mut self, k : Key, style : Style, axis : Axis, source : &S) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.VirtualList, k)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.virtual_count = source.count() as u32; p.virtual_axis = axis; p.has_virtual = true; p.style = style } }
        return id
    }

    func append(&mut self, parent : NodeId, child : NodeId) : bool {
        if(!parent.is_valid() || !child.is_valid() || parent.index() == child.index()) { self.record(ErrorCode.InvalidArgument); return false }
        if(!self.arena.contains(parent) || !self.arena.contains(child)) { self.record(ErrorCode.StaleNode); return false }
        var pp = self.node_ptr(parent)
        var cp = self.node_ptr(child)
        if(pp == null || cp == null) { self.record(ErrorCode.StaleNode); return false }
        unsafe {
            if(cp.parent is Option.Some) { self.record(ErrorCode.InvalidArgument); return false }
            // Reject adding an ancestor as a descendant.
            var cur = parent
            while(cur.is_valid()) {
                if(cur.index() == child.index()) { self.record(ErrorCode.InvalidArgument); return false }
                var np = self.node_ptr(cur)
                if(np == null) { break }
                var par = np.parent
                if(par is Option.None) { break }
                var Some(next) = par else break
                cur = next
            }
            cp.parent = Option.Some<NodeId>(parent)
            cp.next_sibling = VIEW_NONE
            if(pp.last_child == VIEW_NONE) { pp.first_child = child.index(); pp.last_child = child.index() }
            else {
                var lp = self.node_ptr_from_index(pp.last_child)
                if(lp == null) { self.record(ErrorCode.Internal); return false }
                lp.next_sibling = child.index()
                pp.last_child = child.index()
            }
            pp.child_count = pp.child_count + 1u32
            return true
        }
    }

    func node_ptr_from_index(&mut self, index : u32) : *mut ui_internal::ViewNode {
        if(index == 0u32 || index == VIEW_NONE) { return null }
        var ix = (index - 1u32) as size_t
        if(ix >= self.nodes.size()) { return null }
        return self.nodes.get_ptr(ix)
    }

    public func child(&mut self, parent : NodeId, child : NodeId) { var _ = self.append(parent, child) }

    public func children(&mut self, parent : NodeId, kids : &[NodeId]) {
        var i : size_t = 0
        while(i < kids.size()) { var _ = self.append(parent, kids[i]); i = i + 1u }
    }

    public func insert_child(&mut self, parent : NodeId, index : u32, child : NodeId) {
        if(!parent.is_valid() || !child.is_valid() || !self.arena.contains(parent) || !self.arena.contains(child)) { self.record(ErrorCode.StaleNode); return }
        unsafe {
        var pp = self.node_ptr(parent)
        if(pp == null || index > pp.child_count) { self.record(ErrorCode.InvalidArgument); return }
        if(index == pp.child_count) { var _ = self.append(parent, child); return }
        var cp = self.node_ptr(child)
        if(cp == null) { self.record(ErrorCode.StaleNode); return }
        if(cp.parent is Option.Some) { self.record(ErrorCode.InvalidArgument); return }
        var cur = parent
        while(cur.is_valid()) {
            if(cur.index() == child.index()) { self.record(ErrorCode.InvalidArgument); return }
            var np = self.node_ptr(cur)
            if(np == null) { break }
            var par = np.parent
            if(par is Option.None) { break }
            var Some(next) = par else break
            cur = next
        }
        var at = pp.first_child
        var prev = VIEW_NONE
        var n = 0u32
        while(n < index && at != VIEW_NONE) {
            prev = at
            var ap = self.node_ptr_from_index(at)
            if(ap == null) { self.record(ErrorCode.Internal); return }
            at = ap.next_sibling
            n = n + 1u32
        }
        if(at == VIEW_NONE) { var _ = self.append(parent, child); return }
        unsafe {
            cp.parent = Option.Some<NodeId>(parent)
            cp.next_sibling = at
            if(prev == VIEW_NONE) { pp.first_child = child.index() }
            else {
                var vp = self.node_ptr_from_index(prev)
                if(vp == null) { self.record(ErrorCode.Internal); return }
                vp.next_sibling = child.index()
            }
            pp.child_count = pp.child_count + 1u32
        }
        }
    }

    public func style(&mut self, node : NodeId, style : Style) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.style = p.style.merge(style) } } else { self.record(ErrorCode.StaleNode) }
    }
    public func state_style(&mut self, node : NodeId, state : WidgetState, style : Style) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.state_style = p.state_style.merge(style); p.has_state_style = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func command(&mut self, node : NodeId, cmd : CommandId) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.command = cmd; p.has_command = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func semantics(&mut self, node : NodeId, sem : Semantics) {
        var p = self.node_ptr(node)
        if(p != null) {
            unsafe {
                var s = sem
                s.label = self.copy_text(sem.label)
                s.description = self.copy_text(sem.description)
                s.value = self.copy_text(sem.value)
                s.alt = self.copy_text(sem.alt)
                s.lang = self.copy_text(sem.lang)
                s.placeholder = self.copy_text(sem.placeholder)
                s.error = self.copy_text(sem.error)
                p.semantics = s
                p.has_semantics = true
            }
        } else { self.record(ErrorCode.StaleNode) }
    }
    public func test_id(&mut self, node : NodeId, id : string_view) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.test_id = self.copy_text(id) } } else { self.record(ErrorCode.StaleNode) }
    }
    public func focusable(&mut self, node : NodeId, tab_index : i32) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.focusable = true; p.tab_index = tab_index } } else { self.record(ErrorCode.StaleNode) }
    }
    public func grid_cell(&mut self, node : NodeId, col : u16, row : u16, col_span : u16, row_span : u16) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.grid_col = col; p.grid_row = row; p.grid_col_span = col_span; p.grid_row_span = row_span; p.has_grid_cell = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func animate(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing) {
        var spec = ui_internal::empty_spec()
        spec.duration = duration
        spec.easing = easing
        self.animate_spec(node, prop, to, spec)
    }
    public func animate_exit(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing) {
        var spec = ui_internal::empty_spec()
        spec.duration = duration
        spec.easing = easing
        self.animate_exit_spec(node, prop, to, spec)
    }
    public func animate_spec(&mut self, node : NodeId, prop : AnimProperty, to : float, spec : AnimationSpec) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.anim_prop = prop; p.anim_to = to; p.anim_spec = spec; p.has_anim = true; p.anim_exit = false } } else { self.record(ErrorCode.StaleNode) }
    }
    public func animate_exit_spec(&mut self, node : NodeId, prop : AnimProperty, to : float, spec : AnimationSpec) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.anim_prop = prop; p.anim_to = to; p.anim_spec = spec; p.has_anim = true; p.anim_exit = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func transform(&mut self, node : NodeId, t : Transform2D) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.transform = t; p.has_transform = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func error(&mut self, node : NodeId, err : UiError) {
        var p = self.node_ptr(node)
        if(p != null) { unsafe { p.error = err; p.has_error = true } } else { self.record(ErrorCode.StaleNode) }
    }

    func dump_tree(&self) : string {
        var out = string()
        var i : size_t = 0
        while(i < self.nodes.size()) {
            var got = self.nodes.get(i)
            if(got is Option.Some) {
                var Some(n) = got else return out
                var parent_ix = 0u32
                if(n.parent is Option.Some) {
                    var Some(p) = n.parent else return out
                    parent_ix = p.index()
                }
                out.append_view("id="); out.append_integer(n.id.index() as int)
                out.append_view(" gen="); out.append_integer(n.id.generation() as int)
                out.append_view(" kind="); out.append_integer(n.kind as int)
                out.append_view(" parent="); out.append_integer(parent_ix as int)
                out.append_view(" children="); out.append_integer(n.child_count as int)
                out.append_view(" text="); out.append_view(&n.text)
                out.append_view(" test="); out.append_view(&n.test_id)
                out.append('
')
            }
            i = i + 1u
        }
        return out
    }
}

public interface ItemSource {
    func count(&self) : size_t
    func key(&self, index : size_t) : Key
    func item(&self, b : &mut ViewBuilder, index : size_t) : NodeId
    func extent(&self, index : size_t) : float
}

public namespace ui_internal {

public func new_view_builder(capacity : u32, width : float, height : float) : ViewBuilder {
    return ViewBuilder.make_internal(capacity, width, height)
}
public func view_root(b : &ViewBuilder) : NodeId { return b.root }
public func view_node_count(b : &ViewBuilder) : u32 { return b.node_count_value }
public func view_arena_capacity(b : &ViewBuilder) : u32 { return b.arena.capacity() }
public func view_arena_live(b : &ViewBuilder) : u32 { return b.arena.len() }
public func view_frame_high_water(b : &ViewBuilder) : size_t { return b.frame_storage.high_water() }
public func view_frame_allocations(b : &ViewBuilder) : u32 { return b.frame_storage.allocations() }
public func view_diagnostic_count(b : &ViewBuilder) : u32 { return b.diagnostic_count }
public func view_last_diagnostic(b : &ViewBuilder) : ErrorCode { return b.diagnostic_code }
public func view_dump(b : &ViewBuilder) : string { return b.dump_tree() }
public func view_node_kind(b : &ViewBuilder, id : NodeId) : Option<ViewKind> {
    if(!b.arena.contains(id)) { return Option.None<ViewKind>() }
    var ix = (id.index() - 1u32) as size_t
    if(ix >= b.nodes.size()) { return Option.None<ViewKind>() }
    return Option.Some<ViewKind>(unsafe(b.nodes.get_ptr(ix).kind))
}
public func view_parent(b : &ViewBuilder, id : NodeId) : Option<NodeId> {
    if(!b.arena.contains(id)) { return Option.None<NodeId>() }
    var ix = (id.index() - 1u32) as size_t
    if(ix >= b.nodes.size()) { return Option.None<NodeId>() }
    return unsafe(b.nodes.get_ptr(ix).parent)
}
public func view_child_count(b : &ViewBuilder, id : NodeId) : u32 {
    if(!b.arena.contains(id)) { return 0u32 }
    var ix = (id.index() - 1u32) as size_t
    if(ix >= b.nodes.size()) { return 0u32 }
    return unsafe(b.nodes.get_ptr(ix).child_count)
}
public func view_child_at(b : &ViewBuilder, id : NodeId, index : u32) : Option<NodeId> {
    if(!b.arena.contains(id)) { return Option.None<NodeId>() }
    var ix = (id.index() - 1u32) as size_t
    if(ix >= b.nodes.size()) { return Option.None<NodeId>() }
    var n = b.nodes.get_ptr(ix)
    unsafe {
        if(index >= n.child_count) { return Option.None<NodeId>() }
        var cur = n.first_child
        var at = 0u32
        while(at < index && cur != VIEW_NONE) {
            var cp = b.nodes.get_ptr((cur - 1u32) as size_t)
            cur = cp.next_sibling
            at = at + 1u32
        }
        if(cur == VIEW_NONE) { return Option.None<NodeId>() }
        return Option.Some<NodeId>(b.nodes.get_ptr((cur - 1u32) as size_t).id)
    }
}
public func view_text(b : &ViewBuilder, id : NodeId) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).text))
}
public func view_test_id(b : &ViewBuilder, id : NodeId) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).test_id))
}
public func view_style_mask(b : &ViewBuilder, id : NodeId) : u64 {
    if(!b.arena.contains(id)) { return 0u64 }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).style.set_mask)
}
public func view_state_style_mask(b : &ViewBuilder, id : NodeId) : u64 {
    if(!b.arena.contains(id)) { return 0u64 }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).state_style.set_mask)
}
public func view_span_count(b : &ViewBuilder, id : NodeId) : u32 {
    if(!b.arena.contains(id)) { return 0u32 }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).span_count)
}
public func view_span_text(b : &ViewBuilder, id : NodeId, index : u32) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    var n = b.nodes.get_ptr((id.index() - 1u32) as size_t)
    unsafe {
        if(index >= n.span_count) { return Option.None<string_view>() }
        return Option.Some<string_view>(b.spans.get_ptr((n.span_start + index) as size_t).text)
    }
}
public func view_focusable(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).focusable)
}
public func view_has_transform(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).has_transform)
}
public func view_has_animation(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).has_anim)
}
public func view_has_error(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).has_error)
}
public func view_has_semantics(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).has_semantics)
}
public func view_semantics_label(b : &ViewBuilder, id : NodeId) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).semantics.label))
}
public func view_image_placeholder(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = b.nodes.get_ptr((id.index() - 1u32) as size_t)
    unsafe {
        switch(n.image) {
            Path(path) => { return false }
            Bytes(bytes) => { return false }
            Handle(h) => { return false }
            Placeholder() => { return true }
        }
    }
    return false
}
public func view_has_command(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).has_command)
}
public func view_grid_cell(b : &ViewBuilder, id : NodeId) : Option<u32> {
    if(!b.arena.contains(id)) { return Option.None<u32>() }
    var n = b.nodes.get_ptr((id.index() - 1u32) as size_t)
    unsafe { if(!n.has_grid_cell) { return Option.None<u32>() }; return Option.Some<u32>(((n.grid_col as u32) << 16u32) | (n.grid_row as u32)) }
}
public func view_virtual_count(b : &ViewBuilder, id : NodeId) : u32 {
    if(!b.arena.contains(id)) { return 0u32 }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).virtual_count)
}
public func view_frame_capacity(b : &ViewBuilder) : size_t { return b.frame_storage.capacity() }
public func view_scope_depth(b : &ViewBuilder) : u32 { return b.scope_state.count }
public func component_root_of(c : &Component) : NodeId { return c.root }
public func view_component_root(b : &ViewBuilder, id : NodeId) : NodeId {
    if(!b.arena.contains(id)) { return zeroed<NodeId>() }
    return unsafe(b.nodes.get_ptr((id.index() - 1u32) as size_t).component_root)
}


}
