// P1.3 ViewBuilder and frame-local view nodes. No layout/paint/reconcile.
// Compact base node + side tables; heavy metadata is allocated only when used.
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
const VIEW_FLAG_VIRTUAL : u32 = 1u32

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
    var parent_index : u32
    var first_child : u32
    var last_child : u32
    var next_sibling : u32
    var child_count : u32
    var style_index : u32
    var state_style_index : u32
    var meta_index : u32
    var text : string_view
    var virtual_count : u32
}

@direct_init
public struct NodeMeta {
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
    var error_code_value : ErrorCode
    var has_error : bool
    var semantics : Semantics
    var has_semantics : bool
    var virtual_axis : Axis
    var scroll_anchor : ScrollAnchor
    var span_start : u32
    var span_count : u32
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
        duration : Duration { ms : 0.0f }, easing : Easing.EaseOut,
        delay : Duration { ms : 0.0f }, repeat : 0u32, alternate : false
    }
}

func empty_meta() : NodeMeta {
    return NodeMeta {
        image : ImageSource.Placeholder(), icon : IconId.zero(), canvas : CanvasId.zero(),
        overlay_layer : OverlayLayer.Popover, overlay_anchor : Option.None<NodeId>(),
        component_root : zeroed<NodeId>(), slot_name : "", test_id : "",
        focusable : false, tab_index : 0i32, command : CommandId.zero(), has_command : false,
        grid_col : 0u16, grid_row : 0u16, grid_col_span : 1u16, grid_row_span : 1u16,
        has_grid_cell : false, transform : Transform2D.identity(), has_transform : false,
        anim_prop : AnimProperty.Opacity, anim_to : 0.0f, anim_spec : empty_spec(),
        has_anim : false, anim_exit : false,
        error : UiError.make(ErrorCode.Internal, ""), error_code_value : ErrorCode.Internal,
        has_error : false, semantics : empty_semantics(), has_semantics : false,
        virtual_axis : Axis.Vertical, scroll_anchor : ScrollAnchor.Start(),
        span_start : 0u32, span_count : 0u32
    }
}

func make_node(id : NodeId, kind : ViewKind, k : Key, style_index : u32) : ViewNode {
    return ViewNode {
        id : id, key : k, kind : kind, parent_index : VIEW_NONE,
        first_child : VIEW_NONE, last_child : VIEW_NONE, next_sibling : VIEW_NONE,
        child_count : 0u32, style_index : style_index, state_style_index : VIEW_NONE,
        meta_index : VIEW_NONE, text : "", virtual_count : VIEW_NONE
    }
}

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

} // namespace ui_internal

public struct ScopeGuard {
    internal var owner : *mut ui_internal::ScopeStack
    internal var active : bool

    @delete func delete(&mut self) {
        if(self.active) { unsafe { (*self.owner).pop() } }
    }
}

public struct ViewBuilder {
    var arena : NodeArena
    var frame_storage : FrameArena
    var nodes : vector<ui_internal::ViewNode>
    var node_base : *mut ui_internal::ViewNode
    var node_capacity : u32
    var spans : vector<TextSpan>
    var metas : vector<ui_internal::NodeMeta>
    var scope_state : ui_internal::ScopeStack
    var keys_seen : KeySet
    var debug_duplicate : bool
    var root : NodeId
    var frame_data : FrameInfo
    var viewport_data : Viewport
    var node_count_value : u32
    var diagnostic_count : u32
    var diagnostic_code : ErrorCode
    var style_hits : u32
    var style_misses : u32

    func make_internal(capacity : u32, width : float, height : float, debug_duplicate : bool) : ViewBuilder {
        unsafe {
        var viewport = Viewport {
            size : Size.new(width, height), safe_area : Insets.zero(), breakpoint : Breakpoint.Lg
        }
        var frame = FrameInfo {
            window : zeroed<WindowId>(), viewport : viewport, scale : 1.0f,
            text_scale : 1.0f, theme : ThemeMode.Light, reduced_motion : false,
            locale : "en-US", direction : Direction.Ltr, frame : 0u64,
            now_ms : 0.0, focused : true, occluded : false
        }
        var empty_keys = KeySet { keys : vector<Key>(), occupied : vector<bool>(), mask : 0u, count : 0u32 }
        var b = ViewBuilder {
            arena : NodeArena.new(capacity),
            frame_storage : FrameArena.new(16u * 1024u * 1024u),
            nodes : vector<ui_internal::ViewNode>(),
            node_base : null,
            node_capacity : capacity,
            spans : vector<TextSpan>(),
            metas : vector<ui_internal::NodeMeta>(),
            scope_state : ui_internal::ScopeStack { keys : zeroed<[64]Key>(), count : 0u32 },
            keys_seen : empty_keys,
            debug_duplicate : debug_duplicate,
            root : zeroed<NodeId>(),
            frame_data : frame, viewport_data : viewport,
            node_count_value : 0u32, diagnostic_count : 0u32,
            diagnostic_code : ErrorCode.Other, style_hits : 0u32, style_misses : 0u32
        }
        b.nodes.reserve(capacity as size_t)
        b.node_base = b.nodes.mutable_data()
        b.spans.reserve(64u)
        if(debug_duplicate) { b.keys_seen = KeySet.new(capacity + 1u32) }
        var r = b.add_node(ui_internal::ViewKind.Root, key("root"), 0u32)
        b.root = r
        return b

        }}

    func record(&mut self, code : ErrorCode) {
        self.diagnostic_count = self.diagnostic_count + 1u32
        self.diagnostic_code = code
    }

    func style_index(&mut self, style : Style) : u32 {
        self.style_hits = self.style_hits + 1u32
        return style.id
    }
    func style_at(&self, index : u32) : Style {
        return Style { id : index }
    }

    @inline
    func node_ptr(&mut self, id : NodeId) : *mut ui_internal::ViewNode {
        unsafe {
        if(!self.arena.contains(id)) { return null }
        if(id.index() > self.node_count_value) { return null }
        return self.node_base + (id.index() - 1u32)

        }}

    func node_ptr_at(&mut self, index : u32) : *mut ui_internal::ViewNode {
        unsafe {
        if(index == 0u32 || index == VIEW_NONE || index > self.node_count_value) { return null }
        return self.node_base + (index - 1u32)

        }}

    func meta_for(&mut self, id : NodeId) : *mut ui_internal::NodeMeta {
        var p = self.node_ptr(id)
        if(p == null) { return null }
        var mi = unsafe(p.meta_index)
        if(mi == VIEW_NONE) {
            mi = self.metas.size() as u32
            self.metas.push(ui_internal::empty_meta())
            unsafe { p.meta_index = mi }
        }
        if(mi as size_t >= self.metas.size()) { return null }
        return self.metas.get_ptr(mi as size_t)
    }

    func meta_at(&mut self, index : u32) : *mut ui_internal::NodeMeta {
        if(index == VIEW_NONE || index as size_t >= self.metas.size()) { return null }
        return self.metas.get_ptr(index as size_t)
    }

    @inline
    func add_node(&mut self, kind : ui_internal::ViewKind, k : Key, style_index : u32) : NodeId {
        unsafe {
        var r = self.arena.insert()
        if(r is Result.Err) { self.record(ErrorCode.ResourceExhausted); return zeroed<NodeId>() }
        var Ok(id) = r else return zeroed<NodeId>()
        if(self.node_base == null || id.index() > self.node_capacity) { self.record(ErrorCode.ResourceExhausted); return zeroed<NodeId>() }
        var n = self.node_base + (id.index() - 1u32)
        unsafe {
            n.id = id
            n.key = k
            n.kind = kind
            n.parent_index = VIEW_NONE
            n.first_child = VIEW_NONE
            n.last_child = VIEW_NONE
            n.next_sibling = VIEW_NONE
            n.child_count = 0u32
            n.style_index = style_index
            n.state_style_index = VIEW_NONE
            n.meta_index = VIEW_NONE
            n.text = ""
            n.virtual_count = VIEW_NONE
        }
        self.node_count_value = self.node_count_value + 1u32
        if(self.debug_duplicate) {
            if(!self.keys_seen.insert(k)) { self.record(ErrorCode.DuplicateKey) }
        }
        return id

        }}

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
        } else { out.link = Option.None<string_view>() }
        if(span.features.raw_tags.size() > 0u) {
            var n = span.features.raw_tags.size()
            var rp = self.frame_storage.alloc(n * 4u, 4u)
            if(rp is Result.Ok) {
                var Ok(ptr) = rp else return out
                unsafe { memcpy(ptr as *mut void, span.features.raw_tags.data() as *mut void, n * 4u) }
                out.features.raw_tags = unsafe(std::span<u32>(ptr as *mut u32, n))
            } else { self.record(ErrorCode.OutOfMemory) }
        }
        return out
    }

    public func frame(&self) : &FrameInfo { return &self.frame_data }
    public func viewport(&self) : Viewport { return self.viewport_data }
    public func scroll_offset(&self, k : Key) : Point { return Point.zero() }

    func container(&mut self, kind : ui_internal::ViewKind, k : Key, style : Style) : NodeId {
        return self.add_node(kind, k, self.style_index(style))
    }

    public func row(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Row, k, style) }
    public func column(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Column, k, style) }
    public func stack(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Stack, k, style) }
    public func wrap(&mut self, k : Key, style : Style) : NodeId { return self.container(ui_internal::ViewKind.Wrap, k, style) }

    public func grid(&mut self, k : Key, style : Style, tracks : GridTracks) : NodeId {
        var id = self.container(ui_internal::ViewKind.Grid, k, style)
        // Track arrays are contract-owned until P1.6.
        return id
    }

    public func scroll(&mut self, k : Key, style : Style, axis : Axis, anchor : ScrollAnchor = ScrollAnchor.Start()) : NodeId {
        var id = self.container(ui_internal::ViewKind.Scroll, k, style)
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.virtual_axis = axis; mp.scroll_anchor = anchor } }
        return id
    }

    public func canvas(&mut self, k : Key, style : Style, canvas : CanvasId) : NodeId {
        var id = self.container(ui_internal::ViewKind.Canvas, k, style)
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.canvas = canvas } }
        return id
    }

    @inline
    public func fragment(&mut self, k : Key) : NodeId { return self.add_node(ui_internal::ViewKind.Fragment, k, 0u32) }

    public func scope(&mut self, k : Key) : ScopeGuard {
        self.scope_state.push(k)
        return ScopeGuard { owner : unsafe(&raw mut self.scope_state), active : true }
    }
    func pop_scope(&mut self) { self.scope_state.pop() }
    public func scoped_key(&self, parent : Key, label : string_view) : Key { return key_pair(label, parent.id(), parent.tag()) }

    public func overlay(&mut self, k : Key, layer : OverlayLayer, anchor : Option<NodeId>) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Overlay, k, 0u32)
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.overlay_layer = layer; mp.overlay_anchor = anchor } }
        return id
    }

    public func component(&mut self, k : Key, name : string_view) : Component {
        var id = self.add_node(ui_internal::ViewKind.Component, k, 0u32)
        var n = self.copy_text(name)
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.slot_name = n; mp.component_root = id } }
        return Component { root : id, key_value : k, name_value : n }
    }

    public func slot(&mut self, c : Component, name : string_view) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Slot, c.key_value, 0u32)
        var n = self.copy_text(name)
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.slot_name = n; mp.component_root = c.root } }
        if(c.root.is_valid()) { self.child(c.root, id) }
        return id
    }

    public func text(&mut self, k : Key, value : string_view, style : Style) : NodeId {
        var sidx = self.style_index(style)
        var id = self.add_node(ui_internal::ViewKind.Text, k, sidx)
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.text = self.copy_text(value) } }
        return id
    }

    public func rich_text(&mut self, k : Key, spans : &[TextSpan], style : Style) : NodeId {
        var sidx = self.style_index(style)
        var id = self.add_node(ui_internal::ViewKind.RichText, k, sidx)
        var start = self.spans.size() as u32
        var i : size_t = 0
        while(i < spans.size()) {
            var copied = self.copy_span(spans[i])
            self.spans.push(copied)
            i = i + 1u
        }
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.span_start = start; mp.span_count = self.spans.size() as u32 - start } }
        return id
    }

    public func image(&mut self, k : Key, src : ImageSource, style : Style) : NodeId {
        var sidx = self.style_index(style)
        var id = self.add_node(ui_internal::ViewKind.Image, k, sidx)
        var mp = self.meta_for(id)
        if(mp != null) {
            switch(src) {
                Path(path) => { unsafe { mp.image = ImageSource.Path(self.copy_text(path)) } }
                Bytes(bytes) => {
                    self.record(ErrorCode.Unsupported)
                    unsafe { mp.image = ImageSource.Placeholder() }
                }
                Handle(h) => { unsafe { mp.image = ImageSource.Handle(h) } }
                Placeholder() => { unsafe { mp.image = ImageSource.Placeholder() } }
            }
        }
        return id
    }

    public func icon(&mut self, k : Key, icon : IconId, style : Style) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.Icon, k, self.style_index(style))
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.icon = icon } }
        return id
    }

    public func <S : ItemSource> virtual_list(&mut self, k : Key, style : Style, axis : Axis, source : &S) : NodeId {
        var id = self.add_node(ui_internal::ViewKind.VirtualList, k, self.style_index(style))
        var mp = self.meta_for(id)
        if(mp != null) { unsafe { mp.virtual_axis = axis } }
        var p = self.node_ptr(id)
        if(p != null) { unsafe { p.virtual_count = source.count() as u32 } }
        return id
    }

    @inline
    func append(&mut self, parent : NodeId, child : NodeId) : bool {
        if(!parent.is_valid() || !child.is_valid() || parent.index() == child.index()) { self.record(ErrorCode.InvalidArgument); return false }
        unsafe {
            var pp = self.node_ptr(parent)
            var cp = self.node_ptr(child)
            if(pp == null || cp == null) { self.record(ErrorCode.StaleNode); return false }
            if(cp.parent_index != VIEW_NONE) { self.record(ErrorCode.InvalidArgument); return false }
            if(cp.child_count != 0u32 || cp.parent_index != VIEW_NONE) {
                var cur = parent
                while(cur.is_valid()) {
                    if(cur.index() == child.index()) { self.record(ErrorCode.InvalidArgument); return false }
                    var np = self.node_ptr(cur)
                    if(np == null) { break }
                    if(np.parent_index == VIEW_NONE) { break }
                    var nextp = self.node_ptr_at(np.parent_index)
                    if(nextp == null) { break }
                    cur = nextp.id
                }
            }
            cp.parent_index = parent.index()
            cp.next_sibling = VIEW_NONE
            if(pp.last_child == VIEW_NONE) { pp.first_child = child.index(); pp.last_child = child.index() }
            else {
                var lp = self.node_ptr_at(pp.last_child)
                if(lp == null) { self.record(ErrorCode.Internal); return false }
                lp.next_sibling = child.index()
                pp.last_child = child.index()
            }
            pp.child_count = pp.child_count + 1u32
            return true
        }
    }

    @inline
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
            if(cp.parent_index != VIEW_NONE) { self.record(ErrorCode.InvalidArgument); return }
            if(cp.child_count != 0u32 || cp.parent_index != VIEW_NONE) {
                var cur = parent
                while(cur.is_valid()) {
                    if(cur.index() == child.index()) { self.record(ErrorCode.InvalidArgument); return }
                    var np = self.node_ptr(cur)
                    if(np == null) { break }
                    if(np.parent_index == VIEW_NONE) { break }
                    var nextp = self.node_ptr_at(np.parent_index)
                    if(nextp == null) { break }
                    cur = nextp.id
                }
            }
            var at = pp.first_child
            var prev = VIEW_NONE
            var n = 0u32
            while(n < index && at != VIEW_NONE) {
                prev = at
                var ap = self.node_ptr_at(at)
                if(ap == null) { self.record(ErrorCode.Internal); return }
                at = ap.next_sibling
                n = n + 1u32
            }
            if(at == VIEW_NONE) { var _ = self.append(parent, child); return }
            cp.parent_index = parent.index()
            cp.next_sibling = at
            if(prev == VIEW_NONE) { pp.first_child = child.index() }
            else {
                var vp = self.node_ptr_at(prev)
                if(vp == null) { self.record(ErrorCode.Internal); return }
                vp.next_sibling = child.index()
            }
            pp.child_count = pp.child_count + 1u32
        }
    }

    public func style(&mut self, node : NodeId, style : Style) {
        var p = self.node_ptr(node)
        if(p == null) { self.record(ErrorCode.StaleNode); return }
        var current = self.style_at(unsafe(p.style_index))
        var merged = current.merge(style)
        unsafe { p.style_index = self.style_index(merged) }
    }
    public func state_style(&mut self, node : NodeId, state : WidgetState, style : Style) {
        var p = self.node_ptr(node)
        if(p == null) { self.record(ErrorCode.StaleNode); return }
        var current = Style.new()
        var si = unsafe(p.state_style_index)
        if(si != VIEW_NONE) { current = Style { id : si } }
        var merged = current.merge(style)
        unsafe { p.state_style_index = self.style_index(merged) }
    }
    public func command(&mut self, node : NodeId, cmd : CommandId) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.command = cmd; mp.has_command = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func semantics(&mut self, node : NodeId, sem : Semantics) {
        var mp = self.meta_for(node)
        if(mp == null) { self.record(ErrorCode.StaleNode); return }
        var s = sem
        s.label = self.copy_text(sem.label)
        s.description = self.copy_text(sem.description)
        s.value = self.copy_text(sem.value)
        s.alt = self.copy_text(sem.alt)
        s.lang = self.copy_text(sem.lang)
        s.placeholder = self.copy_text(sem.placeholder)
        s.error = self.copy_text(sem.error)
        unsafe { mp.semantics = s; mp.has_semantics = true }
    }
    public func test_id(&mut self, node : NodeId, id : string_view) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.test_id = self.copy_text(id) } } else { self.record(ErrorCode.StaleNode) }
    }
    public func focusable(&mut self, node : NodeId, tab_index : i32) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.focusable = true; mp.tab_index = tab_index } } else { self.record(ErrorCode.StaleNode) }
    }
    public func grid_cell(&mut self, node : NodeId, col : u16, row : u16, col_span : u16, row_span : u16) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.grid_col = col; mp.grid_row = row; mp.grid_col_span = col_span; mp.grid_row_span = row_span; mp.has_grid_cell = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func animate(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing) {
        var spec = ui_internal::empty_spec(); spec.duration = duration; spec.easing = easing
        self.animate_spec(node, prop, to, spec)
    }
    public func animate_exit(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing) {
        var spec = ui_internal::empty_spec(); spec.duration = duration; spec.easing = easing
        self.animate_exit_spec(node, prop, to, spec)
    }
    public func animate_spec(&mut self, node : NodeId, prop : AnimProperty, to : float, spec : AnimationSpec) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.anim_prop = prop; mp.anim_to = to; mp.anim_spec = spec; mp.has_anim = true; mp.anim_exit = false } } else { self.record(ErrorCode.StaleNode) }
    }
    public func animate_exit_spec(&mut self, node : NodeId, prop : AnimProperty, to : float, spec : AnimationSpec) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.anim_prop = prop; mp.anim_to = to; mp.anim_spec = spec; mp.has_anim = true; mp.anim_exit = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func transform(&mut self, node : NodeId, t : Transform2D) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.transform = t; mp.has_transform = true } } else { self.record(ErrorCode.StaleNode) }
    }
    public func error(&mut self, node : NodeId, err : UiError) {
        var mp = self.meta_for(node)
        if(mp != null) { unsafe { mp.error = err; mp.error_code_value = err.code(); mp.has_error = true } } else { self.record(ErrorCode.StaleNode) }
    }

    func dump_tree(&self) : string {
        unsafe {
        var out = string()
        var i : size_t = 0
        while(i < self.node_count_value as size_t) {
            var n = self.node_base + i
            if(n != null) {
                var parent_ix = n.parent_index
                var test : string_view = ""
                var mi = n.meta_index
                if(mi != VIEW_NONE && (mi as size_t) < self.metas.size()) {
                    test = unsafe(self.metas.get_ptr(mi as size_t).test_id)
                }
                out.append_view("id="); out.append_integer(n.id.index() as int)
                out.append_view(" gen="); out.append_integer(n.id.generation() as int)
                out.append_view(" kind="); out.append_integer(n.kind as int)
                out.append_view(" parent="); out.append_integer(parent_ix as int)
                out.append_view(" children="); out.append_integer(n.child_count as int)
                out.append_view(" text="); out.append_view(&n.text)
                out.append_view(" test="); out.append_view(&test)
                out.append('\n')
            }
            i = i + 1u
        }
        return out

        }}
}

public interface ItemSource {
    func count(&self) : size_t
    func key(&self, index : size_t) : Key
    func item(&self, b : &mut ViewBuilder, index : size_t) : NodeId
    func extent(&self, index : size_t) : float
}

public namespace ui_internal {

public func new_view_builder(capacity : u32, width : float, height : float) : ViewBuilder {
    return ViewBuilder.make_internal(capacity, width, height, false)
}
public func new_view_builder_debug(capacity : u32, width : float, height : float) : ViewBuilder {
    return ViewBuilder.make_internal(capacity, width, height, true)
}
public func component_root_of(c : &Component) : NodeId { return c.root }
public func view_root(b : &ViewBuilder) : NodeId { return b.root }
public func view_node_count(b : &ViewBuilder) : u32 { return b.node_count_value }
public func view_arena_capacity(b : &ViewBuilder) : u32 { return b.arena.capacity() }
public func view_arena_live(b : &ViewBuilder) : u32 { return b.arena.len() }
public func view_frame_high_water(b : &ViewBuilder) : size_t { return b.frame_storage.high_water() }
public func view_frame_allocations(b : &ViewBuilder) : u32 { return b.frame_storage.allocations() }
public func view_frame_capacity(b : &ViewBuilder) : size_t { return b.frame_storage.capacity() }
public func view_diagnostic_count(b : &ViewBuilder) : u32 { return b.diagnostic_count }
public func view_last_diagnostic(b : &ViewBuilder) : ErrorCode { return b.diagnostic_code }
public func view_style_hits(b : &ViewBuilder) : u32 { return b.style_hits }
public func view_style_misses(b : &ViewBuilder) : u32 { return b.style_misses }
public func view_style_pool_size(b : &ViewBuilder) : u32 { return style_store_size() }
public func view_meta_count(b : &ViewBuilder) : u32 { return b.metas.size() as u32 }
public func view_dump(b : &ViewBuilder) : string { return b.dump_tree() }
public func view_node_kind(b : &ViewBuilder, id : NodeId) : Option<ViewKind> {
    if(!b.arena.contains(id)) { return Option.None<ViewKind>() }
    return Option.Some<ViewKind>(unsafe((b.node_base + (id.index() - 1u32)).kind))
}
public func view_parent(b : &ViewBuilder, id : NodeId) : Option<NodeId> {
    if(!b.arena.contains(id)) { return Option.None<NodeId>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.parent_index == VIEW_NONE) { return Option.None<NodeId>() }
        return Option.Some<NodeId>((b.node_base + (n.parent_index - 1u32)).id)
    }
}
public func view_child_count(b : &ViewBuilder, id : NodeId) : u32 {
    if(!b.arena.contains(id)) { return 0u32 }
    return unsafe((b.node_base + (id.index() - 1u32)).child_count)
}
public func view_child_at(b : &ViewBuilder, id : NodeId, index : u32) : Option<NodeId> {
    if(!b.arena.contains(id)) { return Option.None<NodeId>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(index >= n.child_count) { return Option.None<NodeId>() }
        var cur = n.first_child
        var at = 0u32
        while(at < index && cur != VIEW_NONE) {
            var cp = (b.node_base + (cur - 1u32))
            cur = cp.next_sibling
            at = at + 1u32
        }
        if(cur == VIEW_NONE) { return Option.None<NodeId>() }
        return Option.Some<NodeId>((b.node_base + (cur - 1u32)).id)
    }
}
public func view_text(b : &ViewBuilder, id : NodeId) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    return Option.Some<string_view>(unsafe((b.node_base + (id.index() - 1u32)).text))
}
public func view_test_id(b : &ViewBuilder, id : NodeId) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.meta_index == VIEW_NONE) { return Option.None<string_view>() }
        return Option.Some<string_view>(b.metas.get_ptr(n.meta_index as size_t).test_id)
    }
}
public func view_style_mask(b : &ViewBuilder, id : NodeId) : u64 {
    if(!b.arena.contains(id)) { return 0u64 }
    return unsafe(style_mask_by_id((b.node_base + (id.index() - 1u32)).style_index))
}
public func view_state_style_mask(b : &ViewBuilder, id : NodeId) : u64 {
    if(!b.arena.contains(id)) { return 0u64 }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.state_style_index == VIEW_NONE) { return 0u64 }
        return style_mask_by_id(n.state_style_index)
    }
}
public func view_span_count(b : &ViewBuilder, id : NodeId) : u32 {
    if(!b.arena.contains(id)) { return 0u32 }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return 0u32 }; return b.metas.get_ptr(n.meta_index as size_t).span_count }
}
public func view_span_text(b : &ViewBuilder, id : NodeId, index : u32) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.meta_index == VIEW_NONE) { return Option.None<string_view>() }
        var m = b.metas.get_ptr(n.meta_index as size_t)
        if(index >= m.span_count) { return Option.None<string_view>() }
        return Option.Some<string_view>(b.spans.get_ptr((m.span_start + index) as size_t).text)
    }
}
public func view_focusable(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.meta_index == VIEW_NONE) { return false }
        return b.metas.get_ptr(n.meta_index as size_t).focusable
    }
}
public func view_has_transform(b : &ViewBuilder, id : NodeId) : bool { return meta_has_transform(b, id) }
func meta_has_transform(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return false }; return b.metas.get_ptr(n.meta_index as size_t).has_transform }
}
public func view_has_animation(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return false }; return b.metas.get_ptr(n.meta_index as size_t).has_anim }
}
public func view_has_error(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return false }; return b.metas.get_ptr(n.meta_index as size_t).has_error }
}
public func view_has_semantics(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return false }; return b.metas.get_ptr(n.meta_index as size_t).has_semantics }
}
public func view_semantics_label(b : &ViewBuilder, id : NodeId) : Option<string_view> {
    if(!b.arena.contains(id)) { return Option.None<string_view>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return Option.None<string_view>() }; return Option.Some<string_view>(b.metas.get_ptr(n.meta_index as size_t).semantics.label) }
}
public func view_image_placeholder(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.meta_index == VIEW_NONE) { return false }
        switch(b.metas.get_ptr(n.meta_index as size_t).image) {
            Path(path) => return false
            Bytes(bytes) => return false
            Handle(h) => return false
            Placeholder() => return true
        }
    }
    return false
}
public func view_has_command(b : &ViewBuilder, id : NodeId) : bool {
    if(!b.arena.contains(id)) { return false }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return false }; return b.metas.get_ptr(n.meta_index as size_t).has_command }
}
public func view_grid_cell(b : &ViewBuilder, id : NodeId) : Option<u32> {
    if(!b.arena.contains(id)) { return Option.None<u32>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe {
        if(n.meta_index == VIEW_NONE) { return Option.None<u32>() }
        var m = b.metas.get_ptr(n.meta_index as size_t)
        if(!m.has_grid_cell) { return Option.None<u32>() }
        return Option.Some<u32>(((m.grid_col as u32) << 16u32) | (m.grid_row as u32))
    }
}
public func view_virtual_count(b : &ViewBuilder, id : NodeId) : u32 {
    if(!b.arena.contains(id)) { return 0u32 }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.virtual_count == VIEW_NONE) { return 0u32 }; return n.virtual_count }
}
public func view_scope_depth(b : &ViewBuilder) : u32 { return b.scope_state.count }
public func view_component_root(b : &ViewBuilder, id : NodeId) : NodeId {
    if(!b.arena.contains(id)) { return zeroed<NodeId>() }
    var n = unsafe(b.node_base + (id.index() - 1u32))
    unsafe { if(n.meta_index == VIEW_NONE) { return zeroed<NodeId>() }; return b.metas.get_ptr(n.meta_index as size_t).component_root }
}

}
