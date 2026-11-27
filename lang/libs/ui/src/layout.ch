// P1.6 layout v1: row/column/stack, padding/gap, fixed/auto/percent/fill,
// min/max, grow/shrink, alignment, baseline, and basic absolute placement.
// Wrap/grid/scroll semantics are intentionally P1.7; text shaping is P2.3.
using std::Option;
using std::Result;
using std::string_view;
using std::vector;
using std::unordered_map;

public namespace ui_internal {

@direct_init
public struct LayoutWarning {
    var node : u32
    var code : u32
}

@direct_init
public struct LayoutSpec {
    var padding : Insets
    var gap : float
    var width : Dim
    var height : Dim
    var min_width : float
    var max_width : float
    var grow : float
    var shrink : float
    var basis : Dim
    var align_items : Align
    var justify : Justify
    var align_self : Align
    var position : Position
    var inset : Insets
    var wrap_mode : WrapMode
    var z_index : i32
    var overflow : Overflow
}

@direct_init
public struct LayoutNode {
    var id : NodeId
    var key : Key
    var kind : ViewKind
    var parent : u32
    var first_child : u32
    var next_sibling : u32
    var child_count : u32
    var style_id : u32
    var text : string_view
    var spec : LayoutSpec
    var rect : Rect
    var content_width : float
    var content_height : float
    var intrinsic_width : float
    var intrinsic_height : float
    var main_size : float
    var cross_size : float
    var baseline : float
    var baseline_offset : float
    var dirty : bool
    var grid_columns : &[GridTrack]
    var grid_rows : &[GridTrack]
    var grid_gap : float
    var grid_col : u16
    var grid_row : u16
    var grid_col_span : u16
    var grid_row_span : u16
    var has_grid_cell : bool
    var scroll_axis : Axis
    var scroll_anchor : ScrollAnchor
    var overlay_layer : OverlayLayer
    var overlay_anchor : Option<NodeId>
    var is_overlay : bool
    var line_index : u32
    var z_index : i32
    var clip : Rect
    var has_clip : bool
    var scroll_content_width : float
    var scroll_content_height : float
    var scroll_max_x : float
    var scroll_max_y : float
    var scroll_x : float
    var scroll_y : float
    var scrollbar_visible_x : bool
    var scrollbar_visible_y : bool
    var scrollbar_track : Rect
    var scrollbar_thumb_x : Rect
    var scrollbar_thumb_y : Rect
    var scroll_damage : Rect
}

@direct_init
public struct LayoutResult {
    var nodes : vector<LayoutNode>
    var root_index : u32
    var viewport : Size
    var warnings : vector<LayoutWarning>
    var pass_count : u32
    var visited : u32
    var cached : bool
}

@direct_init
public struct LayoutStats {
    var passes : u32
    var visited : u32
    var warnings : u32
    var cached : bool
}

@direct_init
public struct LayoutState {
    var result : LayoutResult
    var last_epoch : u64
    var last_width : float
    var last_height : float
    var valid : bool
    var scroll_x : KeyedSlots
    var scroll_y : KeyedSlots
    var scroll_epoch : u64
    var last_scroll_epoch : u64

    public func new(capacity : u32) : LayoutState {
        var r = LayoutResult {
            nodes : vector<LayoutNode>(), root_index : 0u32,
            viewport : Size.zero(), warnings : vector<LayoutWarning>(),
            pass_count : 0u32, visited : 0u32, cached : false
        }
        r.nodes.reserve((capacity + 1u32) as size_t)
        r.warnings.reserve(64u)
        return LayoutState { result : r, last_epoch : 0u64, last_width : 0.0f,
            last_height : 0.0f, valid : false, scroll_x : KeyedSlots.make(),
            scroll_y : KeyedSlots.make(), scroll_epoch : 0u64, last_scroll_epoch : 0u64 }
    }
}

@direct_init
public struct LayoutContext {
    var text : TextContext
    var viewport : Size
}

public func new_layout_state(capacity : u32) : LayoutState {
    return LayoutState.new(capacity)
}

public func new_layout_context(text : TextContext, w : float, h : float) : LayoutContext {
    return LayoutContext { text : text, viewport : Size.new(w, h) }
}

func layout_scroll_has(st : &LayoutState, k : Key) : bool {
    return st.scroll_x.get(k) is Option.Some
}
public func layout_scroll_offset(st : &LayoutState, k : Key) : Point {
    var x = st.scroll_x.get(k)
    var y = st.scroll_y.get(k)
    var px = 0.0f; var py = 0.0f
    if(x is Option.Some) { var Some(v)=x else return Point.zero(); px = f32_from_bits(v as u32) }
    if(y is Option.Some) { var Some(v)=y else return Point.zero(); py = f32_from_bits(v as u32) }
    return Point { x : px, y : py }
}
public func layout_set_scroll(st : &mut LayoutState, k : Key, p : Point) {
    st.scroll_x.set(k, f32_bits(p.x) as i64)
    st.scroll_y.set(k, f32_bits(p.y) as i64)
    st.scroll_epoch = st.scroll_epoch + 1u64
}
public func layout_scroll_to(st : &mut LayoutState, k : Key, anchor : ScrollAnchor) {
    unsafe {
        var n = layout_find_node(st, k)
        if(n == 0u32) { return }
        var node = st.result.nodes.get_ptr(n as size_t)
        var x = 0.0f; var y = 0.0f
        switch(anchor) {
            End() => { x = node.scroll_max_x; y = node.scroll_max_y }
            Center() => { x = node.scroll_max_x * 0.5f; y = node.scroll_max_y * 0.5f }
            Node(k2) => {
                var a = layout_find_node(st, k2)
                if(a != 0u32) { var an = st.result.nodes.get_ptr(a as size_t); x = an.rect.x - node.rect.x; y = an.rect.y - node.rect.y }
            }
            Start() => {}
        }
        layout_set_scroll(st, k, Point { x : x, y : y })
        node.scroll_x = x; node.scroll_y = y
    }
}
func layout_find_node(st : &LayoutState, k : Key) : u32 { unsafe {
    var i : size_t = 1u
    while(i < st.result.nodes.size()) { var n=st.result.nodes.get_ptr(i); if(n.id.is_valid() && n.key.equals_key(k)) { return i as u32 }; i=i+1u }
    return 0u32
}}


func layout_empty_spec() : LayoutSpec {
    return LayoutSpec {
        padding : Insets.zero(), gap : 0.0f, width : Dim.Auto(),
        height : Dim.Auto(), min_width : 0.0f, max_width : 0.0f,
        grow : 0.0f, shrink : 0.0f, basis : Dim.Auto(),
        align_items : Align.Start, justify : Justify.Start, align_self : Align.Start,
        position : Position.Relative(), inset : Insets.zero(),
        wrap_mode : WrapMode.NoWrap, z_index : 0i32, overflow : Overflow.Visible
    }
}

func layout_empty_node() : LayoutNode {
    return LayoutNode {
        id : zeroed<NodeId>(), key : zeroed<Key>(), kind : ViewKind.Root,
        parent : 0u32, first_child : 0u32, next_sibling : 0u32, child_count : 0u32,
        style_id : 0u32, text : "",
        spec : layout_empty_spec(), rect : Rect.zero(), content_width : 0.0f,
        content_height : 0.0f, intrinsic_width : 0.0f, intrinsic_height : 0.0f,
        main_size : 0.0f, cross_size : 0.0f, baseline : 0.0f,
        baseline_offset : 0.0f, dirty : false,
        grid_columns : zeroed<[0]GridTrack>(), grid_rows : zeroed<[0]GridTrack>(),
        grid_gap : 0.0f, grid_col : 0u16, grid_row : 0u16,
        grid_col_span : 1u16, grid_row_span : 1u16, has_grid_cell : false,
        scroll_axis : Axis.Vertical, scroll_anchor : ScrollAnchor.Start(),
        overlay_layer : OverlayLayer.Tooltip, overlay_anchor : Option.None<NodeId>(),
        is_overlay : false, line_index : 0u32, z_index : 0i32, clip : Rect.zero(), has_clip : false,
        scroll_content_width : 0.0f, scroll_content_height : 0.0f,
        scroll_max_x : 0.0f, scroll_max_y : 0.0f, scroll_x : 0.0f, scroll_y : 0.0f,
        scrollbar_visible_x : false, scrollbar_visible_y : false,
        scrollbar_track : Rect.zero(), scrollbar_thumb_x : Rect.zero(),
        scrollbar_thumb_y : Rect.zero(), scroll_damage : Rect.zero()
    }
}

func layout_warn(state : &mut LayoutState, node : u32, code : u32) {
    if(state.result.warnings.size() >= 1024u) { return }
    state.result.warnings.push(LayoutWarning { node : node, code : code })
}

func layout_finite(state : &mut LayoutState, node : u32, v : float, code : u32) : float {
    if(v != v || v < 0.0f || v > 1000000.0f) {
        layout_warn(state, node, code)
        return 0.0f
    }
    return v
}

func layout_insets(state : &mut LayoutState, node : u32, i : Insets) : Insets {
    return Insets {
        left : layout_finite(state, node, i.left, 10u32),
        top : layout_finite(state, node, i.top, 11u32),
        right : layout_finite(state, node, i.right, 12u32),
        bottom : layout_finite(state, node, i.bottom, 13u32)
    }
}

func layout_dim(state : &mut LayoutState, node : u32, d : Dim) : Dim {
    switch(d) {
        Auto() => return Dim.Auto()
        Fill() => return Dim.Fill()
        Px(v) => return Dim.Px(layout_finite(state, node, v, 20u32))
        Percent(v) => return Dim.Percent(layout_finite(state, node, v, 21u32))
    }
    return Dim.Auto()
}

func layout_make_spec(state : &mut LayoutState, node : u32, r : &ResolvedStyle) : LayoutSpec {
    return LayoutSpec {
        padding : layout_insets(state, node, r.padding),
        gap : layout_finite(state, node, r.gap, 1u32),
        width : layout_dim(state, node, r.width),
        height : layout_dim(state, node, r.height),
        min_width : layout_finite(state, node, r.min_width, 2u32),
        max_width : layout_finite(state, node, r.max_width, 3u32),
        grow : layout_finite(state, node, r.grow, 4u32),
        shrink : layout_finite(state, node, r.shrink, 5u32),
        basis : layout_dim(state, node, r.basis),
        align_items : r.align_items,
        justify : r.justify,
        align_self : r.align_self,
        position : r.position,
        inset : layout_insets(state, node, r.inset),
        wrap_mode : r.wrap, z_index : r.z, overflow : r.overflow
    }
}


func layout_insets_equal(a : Insets, b : Insets) : bool {
    return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom
}
func layout_spec_equal(a : LayoutSpec, b : LayoutSpec) : bool {
    return layout_insets_equal(a.padding,b.padding) && a.gap == b.gap &&
        dim_equal(a.width,b.width) && dim_equal(a.height,b.height) &&
        a.min_width == b.min_width && a.max_width == b.max_width &&
        a.grow == b.grow && a.shrink == b.shrink && dim_equal(a.basis,b.basis) &&
        a.align_items == b.align_items && a.justify == b.justify &&
        a.align_self == b.align_self && pos_equal(a.position,b.position) &&
        layout_insets_equal(a.inset,b.inset)
}
func layout_dim_is_px(d : Dim) : bool { switch(d) { Px(_) => return true; default => return false } }

func layout_is_absolute(p : Position) : bool {
    switch(p) {
        Relative() => return false
        Absolute(_,_,_,_) => return true
    }
    return false
}

func layout_is_text(k : ViewKind) : bool { return k == ViewKind.Text || k == ViewKind.RichText }
func layout_is_row(k : ViewKind) : bool { return k == ViewKind.Row }
func layout_is_overlay(k : ViewKind) : bool { return k == ViewKind.Overlay }
func layout_is_wrap(k : ViewKind) : bool { return k == ViewKind.Wrap }
func layout_is_grid(k : ViewKind) : bool { return k == ViewKind.Grid }
func layout_is_scroll(k : ViewKind) : bool { return k == ViewKind.Scroll }
func layout_is_stack(k : ViewKind) : bool {
    return k == ViewKind.Stack || k == ViewKind.Fragment ||
           k == ViewKind.Component || k == ViewKind.Slot || k == ViewKind.VirtualList
}
func layout_excluded(k : ViewKind, p : Position) : bool {
    return layout_is_overlay(k) || layout_is_absolute(p)
}

func layout_flatten_node(state : &mut LayoutState, b : &ViewBuilder, id : NodeId, parent : u32, theme : &Theme) : u32 {
    if(!id.is_valid()) { return 0u32 }
    var bi = id.index()
    if(bi == 0u32 || bi > b.node_count_value || bi as size_t >= state.result.nodes.size()) { return 0u32 }
    unsafe {
        var vn = b.node_base + (bi - 1u32)
        var n = state.result.nodes.get_ptr(bi as size_t)
        n.id = vn.id
        n.key = vn.key
        n.kind = vn.kind
        n.parent = parent
        n.first_child = 0u32
        n.next_sibling = 0u32
        n.child_count = 0u32
        n.style_id = vn.style_index
        n.text = vn.text
        if(vn.meta_index != VIEW_NONE && (vn.meta_index as size_t) < b.metas.size()) {
            var m = b.metas.get_ptr(vn.meta_index as size_t)
            n.grid_columns = m.grid_columns
            n.grid_rows = m.grid_rows
            n.grid_gap = theme.space(m.grid_gap)
            n.grid_col = m.grid_col
            n.grid_row = m.grid_row
            n.grid_col_span = 1u16
            if(m.grid_col_span != 0u16) { n.grid_col_span = m.grid_col_span }
            n.grid_row_span = 1u16
            if(m.grid_row_span != 0u16) { n.grid_row_span = m.grid_row_span }
            n.has_grid_cell = m.has_grid_cell
            n.scroll_axis = m.virtual_axis
            n.scroll_anchor = m.scroll_anchor
            n.overlay_layer = m.overlay_layer
            n.overlay_anchor = m.overlay_anchor
        }
        n.is_overlay = vn.kind == ViewKind.Overlay
        var rs = style_resolve(Style { id : vn.style_index }, theme)
        n.spec = layout_make_spec(state, bi, &rs)
        n.z_index = n.spec.z_index
        n.dirty = true
        var cur = vn.first_child
        var prev : u32 = 0u32
        while(cur != VIEW_NONE) {
            var cp = b.node_base + (cur - 1u32)
            var ci = layout_flatten_node(state, b, cp.id, bi, theme)
            if(ci != 0u32) {
                if(n.first_child == 0u32) { n.first_child = ci }
                if(prev != 0u32) { state.result.nodes.get_ptr(prev as size_t).next_sibling = ci }
                prev = ci
                n.child_count = n.child_count + 1u32
            }
            cur = cp.next_sibling
        }
        state.result.visited = state.result.visited + 1u32
    }
    return bi
}

func layout_measure_text(ctx : &LayoutContext, n : *mut LayoutNode, text_style : TextStyle) : TextLayoutMetrics {
    unsafe {
        var c = TextConstraints {
            max_width : 0.0f, max_lines : 0u16, wrap : WrapMode.NoWrap,
            overflow : TextOverflow.Clip, tab_size : text_style.tab_size,
            direction : Direction.Ltr
        }
        return ctx.text.measure(n.text, text_style, c)
    }
}

func layout_dim_value(state : &mut LayoutState, node : u32, d : Dim, available : float, fallback : float) : float {
    switch(d) {
        Auto() => return fallback
        Fill() => return fallback
        Px(v) => return layout_finite(state, node, v, 22u32)
        Percent(v) => {
            var p = layout_finite(state, node, v, 23u32)
            if(available <= 0.0f) { return 0.0f }
            return available * p
        }
    }
    return fallback
}

func layout_intrinsic(state : &mut LayoutState, index : u32, ctx : &LayoutContext, theme : &Theme) : Size {
    if(index == 0u32 || index as size_t >= state.result.nodes.size()) { return Size.zero() }
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        if(layout_is_text(n.kind)) {
            var rs = style_resolve(Style { id : n.style_id }, theme)
            var m = layout_measure_text(ctx, n, rs.text)
            var tw = m.width
            var th = m.height
            switch(n.spec.width) {
                Px(v) => { tw = layout_finite(state, index, v, 28u32) }
                default => {}
            }
            switch(n.spec.height) {
                Px(v) => { th = layout_finite(state, index, v, 29u32) }
                default => {}
            }
            n.intrinsic_width = tw
            n.intrinsic_height = th
            n.baseline_offset = m.first_baseline
            return Size.new(tw, th)
        }
        var content_w = 0.0f
        var content_h = 0.0f
        var count = 0u32
        var cur = n.first_child
        var row = layout_is_row(n.kind)
        var stack = layout_is_stack(n.kind)
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                var cs = layout_intrinsic(state, cur, ctx, theme)
                if(stack) {
                    if(cs.width > content_w) { content_w = cs.width }
                    if(cs.height > content_h) { content_h = cs.height }
                } else if(row) {
                    content_w = content_w + cs.width
                    if(cs.height > content_h) { content_h = cs.height }
                } else {
                    content_h = content_h + cs.height
                    if(cs.width > content_w) { content_w = cs.width }
                }
                count = count + 1u32
            }
            cur = cp.next_sibling
        }
        if(!stack && count > 1u32) {
            var gap = layout_finite(state, index, n.spec.gap, 6u32)
            if(row) { content_w = content_w + gap * ((count - 1u32) as float) }
            else { content_h = content_h + gap * ((count - 1u32) as float) }
        }
        var total_w = content_w + n.spec.padding.left + n.spec.padding.right
        var total_h = content_h + n.spec.padding.top + n.spec.padding.bottom
        switch(n.spec.width) {
            Px(v) => { total_w = layout_finite(state, index, v, 24u32) }
            default => {}
        }
        switch(n.spec.height) {
            Px(v) => { total_h = layout_finite(state, index, v, 25u32) }
            default => {}
        }
        if(n.spec.min_width > 0.0f && total_w < n.spec.min_width) { total_w = n.spec.min_width }
        if(n.spec.max_width > 0.0f && total_w > n.spec.max_width) { total_w = n.spec.max_width }
        n.intrinsic_width = total_w
        n.intrinsic_height = total_h
        n.baseline_offset = total_h
        return Size.new(total_w, total_h)
    }
}

func layout_main_base(state : &mut LayoutState, index : u32, parent_main : float, row : bool) : float {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var d = n.spec.height
        if(row) { d = n.spec.width }
        switch(n.spec.basis) { Auto() => {} default => { d = n.spec.basis } }
        var base = 0.0f
        switch(d) {
            Auto() => {
                base = n.intrinsic_height
                if(row) { base = n.intrinsic_width }
            }
            Fill() => {
                base = 0.0f
                if(n.spec.grow <= 0.0f) { n.spec.grow = 1.0f }
            }
            Px(v) => { base = layout_finite(state, index, v, 26u32) }
            Percent(v) => {
                var p = layout_finite(state, index, v, 27u32)
                if(parent_main > 0.0f) { base = parent_main * p } else { base = 0.0f }
            }
        }
        if(row) {
            if(n.spec.min_width > 0.0f && base < n.spec.min_width) { base = n.spec.min_width }
            if(n.spec.max_width > 0.0f && base > n.spec.max_width) { base = n.spec.max_width }
        }
        n.main_size = base
        return base
    }
}

func layout_cross_size(state : &mut LayoutState, index : u32, content_cross : float, row : bool, parent_align : Align) : float {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var d = n.spec.width
        var fallback = n.intrinsic_width
        if(row) { d = n.spec.height; fallback = n.intrinsic_height }
        var size = layout_dim_value(state, index, d, content_cross, fallback)
        var stretch = n.spec.align_self == Align.Stretch || (n.spec.align_self == Align.Start && parent_align == Align.Stretch)
        switch(d) {
            Auto() => { if(stretch) { size = content_cross } }
            default => {}
        }
        if(!row) {
            if(n.spec.min_width > 0.0f && size < n.spec.min_width) { size = n.spec.min_width }
            if(n.spec.max_width > 0.0f && size > n.spec.max_width) { size = n.spec.max_width }
        }
        n.cross_size = size
        return size
    }
}

func layout_align_offset(align : Align, content : float, size : float, baseline : float, max_baseline : float) : float {
    switch(align) {
        Align.Start => return 0.0f
        Align.Center => return (content - size) * 0.5f
        Align.End => return content - size
        Align.Stretch => return 0.0f
        Align.Baseline => return max_baseline - baseline
    }
    return 0.0f
}

func layout_justify_extra(justify : Justify, remaining : float, count : u32) : float {
    if(remaining <= 0.0f || count == 0u32) { return 0.0f }
    var n = count as float
    switch(justify) {
        Justify.Start => return 0.0f
        Justify.Center => return remaining * 0.5f
        Justify.End => return remaining
        Justify.SpaceBetween => { if(count <= 1u32) { return 0.0f }; return remaining / (n - 1.0f) }
        Justify.SpaceAround => return remaining / (n * 2.0f)
        Justify.SpaceEvenly => return remaining / (n + 1.0f)
    }
    return 0.0f
}

func layout_place_linear(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var row = layout_is_row(n.kind)
        var content = Rect.new(n.rect.x + n.spec.padding.left, n.rect.y + n.spec.padding.top,
            n.rect.w - n.spec.padding.left - n.spec.padding.right,
            n.rect.h - n.spec.padding.top - n.spec.padding.bottom)
        n.content_width = content.w
        n.content_height = content.h
        var main = content.h
        var cross = content.w
        if(row) { main = content.w; cross = content.h }
        var count = 0u32
        var total = 0.0f
        var cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                total = total + layout_main_base(state, cur, main, row)
                count = count + 1u32
            }
            cur = cp.next_sibling
        }
        var gap = n.spec.gap
        if(count > 1u32) { total = total + gap * ((count - 1u32) as float) }
        var remaining = main - total
        if(remaining > 0.0f) {
            var weight = 0.0f
            cur = n.first_child
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position)) { weight = weight + cp.spec.grow }
                cur = cp.next_sibling
            }
            if(weight > 0.0f) {
                cur = n.first_child
                while(cur != 0u32) {
                    var cp = state.result.nodes.get_ptr(cur as size_t)
                    if(!layout_excluded(cp.kind, cp.spec.position)) { cp.main_size = cp.main_size + remaining * cp.spec.grow / weight }
                    cur = cp.next_sibling
                }
                remaining = 0.0f
            }
        } else if(remaining < 0.0f) {
            var weight = 0.0f
            cur = n.first_child
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position)) { weight = weight + cp.spec.shrink * cp.main_size }
                cur = cp.next_sibling
            }
            if(weight > 0.0f) {
                cur = n.first_child
                while(cur != 0u32) {
                    var cp = state.result.nodes.get_ptr(cur as size_t)
                    if(!layout_excluded(cp.kind, cp.spec.position)) {
                        var cut = (0.0f - remaining) * (cp.spec.shrink * cp.main_size) / weight
                        cp.main_size = cp.main_size - cut
                        if(cp.main_size < 0.0f) { cp.main_size = 0.0f }
                    }
                    cur = cp.next_sibling
                }
            }
            remaining = 0.0f
        }
        var extra = layout_justify_extra(n.spec.justify, remaining, count)
        var cursor = content.y
        if(row) { cursor = content.x }
        var max_baseline = 0.0f
        if(row && n.spec.align_items == Align.Baseline) {
            cur = n.first_child
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position) && cp.baseline_offset > max_baseline) { max_baseline = cp.baseline_offset }
                cur = cp.next_sibling
            }
        }
        cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                var cs = layout_cross_size(state, cur, cross, row, n.spec.align_items)
                var off = layout_align_offset(cp.spec.align_self, cross, cs, cp.baseline_offset, max_baseline)
                if(row && cp.spec.align_self == Align.Start) { off = layout_align_offset(n.spec.align_items, cross, cs, cp.baseline_offset, max_baseline) }
                var x = content.x + off
                var y = content.y + off
                var w = cs
                var h = cp.main_size
                if(row) { x = cursor; y = content.y + off; w = cp.main_size; h = cs }
                else { x = content.x + off; y = cursor }
                cp.rect = Rect.new(x, y, w, h)
                state.result.nodes.get_ptr(cur as size_t).baseline = y + cp.baseline_offset
                layout_place(state, cur, ctx)
                cursor = cursor + cp.main_size + gap + extra
            }
            cur = cp.next_sibling
        }
        n.baseline_offset = n.rect.h
        if(row) { n.baseline_offset = n.spec.padding.top + max_baseline }
        n.baseline = n.rect.y + n.baseline_offset
        n.dirty = false
    }
}

func layout_place_stack(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var content = Rect.new(n.rect.x + n.spec.padding.left, n.rect.y + n.spec.padding.top,
            n.rect.w - n.spec.padding.left - n.spec.padding.right,
            n.rect.h - n.spec.padding.top - n.spec.padding.bottom)
        n.content_width = content.w
        n.content_height = content.h
        var cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                var w = layout_dim_value(state, cur, cp.spec.width, content.w, cp.intrinsic_width)
                var h = layout_dim_value(state, cur, cp.spec.height, content.h, cp.intrinsic_height)
                if(cp.spec.min_width > 0.0f && w < cp.spec.min_width) { w = cp.spec.min_width }
                if(cp.spec.max_width > 0.0f && w > cp.spec.max_width) { w = cp.spec.max_width }
                cp.rect = Rect.new(content.x, content.y, w, h)
                cp.baseline = content.y + cp.baseline_offset
                layout_place(state, cur, ctx)
            }
            cur = cp.next_sibling
        }
        n.baseline_offset = n.rect.h
        n.baseline = n.rect.y + n.rect.h
        n.dirty = false
    }
}

func layout_place_absolute(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var content = Rect.new(n.rect.x + n.spec.padding.left, n.rect.y + n.spec.padding.top,
            n.rect.w - n.spec.padding.left - n.spec.padding.right,
            n.rect.h - n.spec.padding.top - n.spec.padding.bottom)
        var top = 0.0f; var right = 0.0f; var bottom = 0.0f; var left = 0.0f
        switch(n.spec.position) {
            Absolute(t,r,b,l) => {
                top = layout_dim_value(state, index, t, content.h, 0.0f)
                right = layout_dim_value(state, index, r, content.w, 0.0f)
                bottom = layout_dim_value(state, index, b, content.h, 0.0f)
                left = layout_dim_value(state, index, l, content.w, 0.0f)
            }
            Relative() => {}
        }
        var w = layout_dim_value(state, index, n.spec.width, content.w, n.intrinsic_width)
        var h = layout_dim_value(state, index, n.spec.height, content.h, n.intrinsic_height)
        n.rect = Rect.new(content.x + left, content.y + top, w, h)
        n.baseline = n.rect.y + n.baseline_offset
        layout_place(state, index, ctx)
    }
}

func layout_shift_subtree(state : &mut LayoutState, index : u32, dx : float, dy : float) {
    if(index == 0u32 || index as size_t >= state.result.nodes.size()) { return }
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        n.rect.x = n.rect.x + dx
        n.rect.y = n.rect.y + dy
        if(n.has_clip) { n.clip.x = n.clip.x + dx; n.clip.y = n.clip.y + dy }
        var cur = n.first_child
        while(cur != 0u32) {
            layout_shift_subtree(state, cur, dx, dy)
            cur = state.result.nodes.get_ptr(cur as size_t)[0].next_sibling
        }
    }
}

func layout_place_overlay(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var p = state.result.nodes.get_ptr(n.parent as size_t)
        if(p == null) { return }
        var base = Rect.new(p.rect.x + p.spec.padding.left, p.rect.y + p.spec.padding.top,
            p.rect.w - p.spec.padding.left - p.spec.padding.right,
            p.rect.h - p.spec.padding.top - p.spec.padding.bottom)
        if(n.overlay_anchor is Option.Some) {
            var Some(aid)=n.overlay_anchor else return
            if(aid.is_valid() && (aid.index() as size_t) < state.result.nodes.size()) { base = state.result.nodes.get_ptr(aid.index() as size_t)[0].rect }
        }
        var w = layout_dim_value(state, index, n.spec.width, base.w, n.intrinsic_width)
        var h = layout_dim_value(state, index, n.spec.height, base.h, n.intrinsic_height)
        if(n.spec.min_width > 0.0f && w < n.spec.min_width) { w = n.spec.min_width }
        if(n.spec.max_width > 0.0f && w > n.spec.max_width) { w = n.spec.max_width }
        n.rect = Rect.new(base.x, base.y, w, h)
        n.baseline_offset = n.rect.h
        n.baseline = n.rect.y + n.rect.h
        n.dirty = false
        layout_place(state, index, ctx)
    }
}

func layout_place_scroll(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var content = Rect.new(n.rect.x + n.spec.padding.left, n.rect.y + n.spec.padding.top,
            n.rect.w - n.spec.padding.left - n.spec.padding.right,
            n.rect.h - n.spec.padding.top - n.spec.padding.bottom)
        n.content_width = content.w
        n.content_height = content.h
        n.clip = content
        n.has_clip = true
        var normal_count = 0u32
        var only_child = 0u32
        var cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                normal_count = normal_count + 1u32
                only_child = cur
                var w = layout_dim_value(state, cur, cp.spec.width, content.w, cp.intrinsic_width)
                var h = layout_dim_value(state, cur, cp.spec.height, content.h, cp.intrinsic_height)
                if(cp.spec.min_width > 0.0f && w < cp.spec.min_width) { w = cp.spec.min_width }
                if(cp.spec.max_width > 0.0f && w > cp.spec.max_width) { w = cp.spec.max_width }
                cp.rect = Rect.new(content.x, content.y, w, h)
                cp.baseline_offset = h
                cp.baseline = content.y + h
                layout_place(state, cur, ctx)
            }
            cur = cp.next_sibling
        }
        var extent_w = content.w
        var extent_h = content.h
        if(normal_count == 1u32) {
            var cp = state.result.nodes.get_ptr(only_child as size_t)
            var ew = cp.rect.x2() - content.x
            var eh = cp.rect.y2() - content.y
            if(ew > extent_w) { extent_w = ew }
            if(eh > extent_h) { extent_h = eh }
        } else if(normal_count > 1u32 || normal_count == 0u32) {
            cur = n.first_child
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position)) {
                    var ew = cp.rect.x2() - content.x
                    var eh = cp.rect.y2() - content.y
                    if(ew > extent_w) { extent_w = ew }
                    if(eh > extent_h) { extent_h = eh }
                }
                cur = cp.next_sibling
            }
        }
        n.scroll_content_width = extent_w
        n.scroll_content_height = extent_h
        n.scroll_max_x = 0.0f
        n.scroll_max_y = 0.0f
        if(n.scroll_axis == Axis.Horizontal || n.scroll_axis == Axis.Both) { n.scroll_max_x = max32(0.0f, extent_w - content.w) }
        if(n.scroll_axis == Axis.Vertical || n.scroll_axis == Axis.Both) { n.scroll_max_y = max32(0.0f, extent_h - content.h) }
        var off = layout_scroll_offset(state, n.key)
        if(!layout_scroll_has(state, n.key)) {
            switch(n.scroll_anchor) {
                End() => { off.x = n.scroll_max_x; off.y = n.scroll_max_y }
                Center() => { off.x = n.scroll_max_x * 0.5f; off.y = n.scroll_max_y * 0.5f }
                Node(k2) => {
                    var a = layout_find_node(state, k2)
                    if(a != 0u32) { var an = state.result.nodes.get_ptr(a as size_t); off.x = an.rect.x - content.x; off.y = an.rect.y - content.y }
                }
                Start() => {}
            }
        }
        if(off.x < 0.0f) { off.x = 0.0f }
        if(off.y < 0.0f) { off.y = 0.0f }
        if(off.x > n.scroll_max_x) { off.x = n.scroll_max_x }
        if(off.y > n.scroll_max_y) { off.y = n.scroll_max_y }
        n.scroll_x = off.x; n.scroll_y = off.y
        layout_set_scroll(state, n.key, off)
        if(off.x != 0.0f || off.y != 0.0f) {
            cur = n.first_child
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position)) { layout_shift_subtree(state, cur, -off.x, -off.y) }
                cur = cp.next_sibling
            }
        }
        n.scrollbar_visible_x = n.scroll_max_x > 0.0f
        n.scrollbar_visible_y = n.scroll_max_y > 0.0f
        n.scrollbar_track = Rect.zero()
        n.scrollbar_thumb_x = Rect.zero()
        n.scrollbar_thumb_y = Rect.zero()
        if(n.scrollbar_visible_y) {
            var tw = 6.0f
            n.scrollbar_track = Rect.new(content.x + content.w - tw, content.y, tw, content.h)
            var th = content.h * content.h / max32(content.h, n.scroll_content_height)
            if(th < 20.0f) { th = 20.0f }
            if(th > content.h) { th = content.h }
            var ty = content.y
            if(n.scroll_max_y > 0.0f) { ty = content.y + (off.y / n.scroll_max_y) * (content.h - th) }
            n.scrollbar_thumb_y = Rect.new(n.scrollbar_track.x, ty, tw, th)
        }
        if(n.scrollbar_visible_x) {
            var th = 6.0f
            n.scrollbar_track = Rect.new(content.x, content.y + content.h - th, content.w, th)
            var tw = content.w * content.w / max32(content.w, n.scroll_content_width)
            if(tw < 20.0f) { tw = 20.0f }
            if(tw > content.w) { tw = content.w }
            var tx = content.x
            if(n.scroll_max_x > 0.0f) { tx = content.x + (off.x / n.scroll_max_x) * (content.w - tw) }
            n.scrollbar_thumb_x = Rect.new(tx, n.scrollbar_track.y, tw, th)
        }
        n.scroll_damage = content
        n.baseline_offset = n.rect.h
        n.baseline = n.rect.y + n.rect.h
        n.dirty = false
    }
}

func layout_place_wrap(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var content = Rect.new(n.rect.x + n.spec.padding.left, n.rect.y + n.spec.padding.top,
            n.rect.w - n.spec.padding.left - n.spec.padding.right,
            n.rect.h - n.spec.padding.top - n.spec.padding.bottom)
        n.content_width = content.w
        n.content_height = content.h
        var gap = n.spec.gap
        var line_widths = vector<float>()
        var line_heights = vector<float>()
        var line_weights = vector<float>()
        var line_shrinks = vector<float>()
        var line_counts = vector<u32>()
        var line_extra = vector<float>()
        var line = 0u32
        var line_width = 0.0f
        var line_height = 0.0f
        var line_weight = 0.0f
        var line_shrink = 0.0f
        var line_count = 0u32
        var cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                var base = layout_main_base(state, cur, content.w, true)
                var cs = layout_cross_size(state, cur, content.h, true, n.spec.align_items)
                if(line_count > 0u32 && line_width + gap + base > content.w) {
                    line_widths.push(line_width)
                    line_heights.push(line_height)
                    line_weights.push(line_weight)
                    line_shrinks.push(line_shrink)
                    line_counts.push(line_count)
                    line = line + 1u32
                    line_width = 0.0f; line_height = 0.0f; line_weight = 0.0f; line_shrink = 0.0f; line_count = 0u32
                }
                cp.line_index = line
                cp.main_size = base
                cp.cross_size = cs
                if(line_count > 0u32) { line_width = line_width + gap }
                line_width = line_width + base
                if(cs > line_height) { line_height = cs }
                line_weight = line_weight + cp.spec.grow
                line_shrink = line_shrink + cp.spec.shrink * base
                line_count = line_count + 1u32
            }
            cur = cp.next_sibling
        }
        if(line_count > 0u32) {
            line_widths.push(line_width)
            line_heights.push(line_height)
            line_weights.push(line_weight)
            line_shrinks.push(line_shrink)
            line_counts.push(line_count)
        }
        var total_h = 0.0f
        var li : size_t = 0u
        while(li < line_heights.size()) { total_h = total_h + line_heights.get_ptr(li)[0]; if(li + 1u < line_heights.size()) { total_h = total_h + gap }; li = li + 1u }
        li = 0u
        while(li < line_widths.size()) {
            var lw = line_widths.get_ptr(li)[0]
            var rem = content.w - lw
            var ex = 0.0f
            if(rem > 0.0f && line_weights.get_ptr(li)[0] <= 0.0f) { ex = layout_justify_extra(n.spec.justify, rem, line_counts.get_ptr(li)[0]) }
            line_extra.push(ex)
            li = li + 1u
        }
        var y_cursor = content.y
        if(n.spec.wrap_mode == WrapMode.WrapReverse) { y_cursor = content.y + total_h }
        var x_cursor = content.x
        var prev_line = 0u32
        li = 0u
        while(li < line_heights.size()) {
            var lh = line_heights.get_ptr(li)[0]
            var ly = y_cursor
            if(n.spec.wrap_mode == WrapMode.WrapReverse) { ly = y_cursor - lh; y_cursor = ly - gap }
            else { y_cursor = y_cursor + lh + gap }
            x_cursor = content.x
            var lw = line_widths.get_ptr(li)[0]
            var rem = content.w - lw
            var over = 0.0f
            if(rem < 0.0f) { over = 0.0f - rem }
            var weight = line_weights.get_ptr(li)[0]
            var shrink_w = line_shrinks.get_ptr(li)[0]
            var ex = line_extra.get_ptr(li)[0]
            cur = n.first_child
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position) && cp.line_index == li as u32) {
                    var main = cp.main_size
                    if(rem > 0.0f && weight > 0.0f) { main = main + rem * cp.spec.grow / weight }
                    if(over > 0.0f && shrink_w > 0.0f) { main = main - over * (cp.spec.shrink * cp.main_size) / shrink_w; if(main < 0.0f) { main = 0.0f } }
                    var cs = cp.cross_size
                    var off = layout_align_offset(cp.spec.align_self, lh, cs, cp.baseline_offset, lh)
                    if(cp.spec.align_self == Align.Start) { off = layout_align_offset(n.spec.align_items, lh, cs, cp.baseline_offset, lh) }
                    cp.rect = Rect.new(x_cursor, ly + off, main, cs)
                    cp.baseline = cp.rect.y + cp.baseline_offset
                    layout_place(state, cur, ctx)
                    x_cursor = x_cursor + main + gap + ex
                }
                cur = cp.next_sibling
            }
            li = li + 1u
        }
        n.baseline_offset = n.rect.h
        n.baseline = n.rect.y + n.rect.h
        n.dirty = false
    }
}

func layout_place_grid(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var content = Rect.new(n.rect.x + n.spec.padding.left, n.rect.y + n.spec.padding.top,
            n.rect.w - n.spec.padding.left - n.spec.padding.right,
            n.rect.h - n.spec.padding.top - n.spec.padding.bottom)
        n.content_width = content.w
        n.content_height = content.h
        var cols = n.grid_columns.size() as u32
        var rows = n.grid_rows.size() as u32
        if(cols == 0u32) { cols = 1u32 }
        if(rows == 0u32) { rows = 1u32 }
        var has_auto_cols = false
        var has_auto_rows = false
        var widths = vector<float>(); widths.resize(cols as size_t)
        var heights = vector<float>(); heights.resize(rows as size_t)
        var cfr = vector<float>(); cfr.resize(cols as size_t)
        var rfr = vector<float>(); rfr.resize(rows as size_t)
        var c = 0u32
        while(c < cols) {
            var t = n.grid_columns[c as size_t]
            switch(t) {
                Px(v) => { widths.get_ptr(c as size_t)[0] = layout_finite(state, index, v, 40u32) }
                Fr(v) => { cfr.get_ptr(c as size_t)[0] = layout_finite(state, index, v, 41u32) }
                Auto() => { has_auto_cols = true }
            }
            c = c + 1u32
        }
        var r = 0u32
        while(r < rows) {
            var t = n.grid_rows[r as size_t]
            switch(t) {
                Px(v) => { heights.get_ptr(r as size_t)[0] = layout_finite(state, index, v, 42u32) }
                Fr(v) => { rfr.get_ptr(r as size_t)[0] = layout_finite(state, index, v, 43u32) }
                Auto() => { has_auto_rows = true }
            }
            r = r + 1u32
        }
        // Auto tracks resolve from intrinsic child sizes only when needed.
        var cur = n.first_child
        if(has_auto_cols || has_auto_rows) {
            while(cur != 0u32) {
                var cp = state.result.nodes.get_ptr(cur as size_t)
                if(!layout_excluded(cp.kind, cp.spec.position)) {
                    var cc = cp.grid_col as u32
                    var rr = cp.grid_row as u32
                    if(has_auto_cols && cc < cols && cp.intrinsic_width > widths.get_ptr(cc as size_t)[0]) { widths.get_ptr(cc as size_t)[0] = cp.intrinsic_width }
                    if(has_auto_rows && rr < rows && cp.intrinsic_height > heights.get_ptr(rr as size_t)[0]) { heights.get_ptr(rr as size_t)[0] = cp.intrinsic_height }
                }
                cur = cp.next_sibling
            }
        }
        var gap = n.grid_gap
        var fixed_w = 0.0f; var free_w = content.w
        c = 0u32; while(c < cols) { fixed_w = fixed_w + widths.get_ptr(c as size_t)[0]; c = c + 1u32 }
        if(cols > 1u32) { fixed_w = fixed_w + gap * ((cols - 1u32) as float) }
        free_w = content.w - fixed_w
        var cweight = 0.0f; c = 0u32; while(c < cols) { cweight = cweight + cfr.get_ptr(c as size_t)[0]; c = c + 1u32 }
        if(free_w > 0.0f && cweight > 0.0f) { c = 0u32; while(c < cols) { widths.get_ptr(c as size_t)[0] = widths.get_ptr(c as size_t)[0] + free_w * cfr.get_ptr(c as size_t)[0] / cweight; c = c + 1u32 } }
        var fixed_h = 0.0f
        r = 0u32; while(r < rows) { fixed_h = fixed_h + heights.get_ptr(r as size_t)[0]; r = r + 1u32 }
        if(rows > 1u32) { fixed_h = fixed_h + gap * ((rows - 1u32) as float) }
        var free_h = content.h - fixed_h
        var rweight = 0.0f; r = 0u32; while(r < rows) { rweight = rweight + rfr.get_ptr(r as size_t)[0]; r = r + 1u32 }
        if(free_h > 0.0f && rweight > 0.0f) { r = 0u32; while(r < rows) { heights.get_ptr(r as size_t)[0] = heights.get_ptr(r as size_t)[0] + free_h * rfr.get_ptr(r as size_t)[0] / rweight; r = r + 1u32 } }
        cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(!layout_excluded(cp.kind, cp.spec.position)) {
                var cc = cp.grid_col as u32; var rr = cp.grid_row as u32
                if(cc >= cols) { cc = cols - 1u32 }
                if(rr >= rows) { rr = rows - 1u32 }
                var cspan = cp.grid_col_span as u32; if(cspan == 0u32) { cspan = 1u32 }
                var rspan = cp.grid_row_span as u32; if(rspan == 0u32) { rspan = 1u32 }
                if(cc + cspan > cols) { cspan = cols - cc }
                if(rr + rspan > rows) { rspan = rows - rr }
                var x = content.x; var y = content.y
                var cix = 0u32; while(cix < cc) { x = x + widths.get_ptr(cix as size_t)[0] + gap; cix = cix + 1u32 }
                var rix = 0u32; while(rix < rr) { y = y + heights.get_ptr(rix as size_t)[0] + gap; rix = rix + 1u32 }
                var w = 0.0f; cix = 0u32; while(cix < cspan) { w = w + widths.get_ptr((cc + cix) as size_t)[0]; if(cix + 1u32 < cspan) { w = w + gap }; cix = cix + 1u32 }
                var h = 0.0f; rix = 0u32; while(rix < rspan) { h = h + heights.get_ptr((rr + rix) as size_t)[0]; if(rix + 1u32 < rspan) { h = h + gap }; rix = rix + 1u32 }
                cp.rect = Rect.new(x, y, w, h)
                cp.baseline = y + h
                layout_place(state, cur, ctx)
            }
            cur = cp.next_sibling
        }
        n.baseline_offset = n.rect.h
        n.baseline = n.rect.y + n.rect.h
        n.dirty = false
    }
}


func layout_place(state : &mut LayoutState, index : u32, ctx : &LayoutContext) {
    if(index == 0u32 || index as size_t >= state.result.nodes.size()) { return }
    var n = state.result.nodes.get_ptr(index as size_t)
    unsafe {
        var content_w = n.rect.w - n.spec.padding.left - n.spec.padding.right
        var content_h = n.rect.h - n.spec.padding.top - n.spec.padding.bottom
        if(content_w < 0.0f) { content_w = 0.0f }
        if(content_h < 0.0f) { content_h = 0.0f }
        n.content_width = content_w
        n.content_height = content_h
        if(layout_is_text(n.kind)) {
            n.baseline = n.rect.y + n.baseline_offset
            n.dirty = false
        } else if(n.child_count == 0u32) {
            n.baseline_offset = n.rect.h
            n.baseline = n.rect.y + n.rect.h
            n.dirty = false
        } else if(layout_is_wrap(n.kind)) {
            layout_place_wrap(state, index, ctx)
        } else if(layout_is_grid(n.kind)) {
            layout_place_grid(state, index, ctx)
        } else if(layout_is_scroll(n.kind)) {
            layout_place_scroll(state, index, ctx)
        } else if(layout_is_stack(n.kind)) {
            layout_place_stack(state, index, ctx)
        } else {
            layout_place_linear(state, index, ctx)
        }
        // Absolute and overlay children are placed outside the normal flow.
        var cur = n.first_child
        while(cur != 0u32) {
            var cp = state.result.nodes.get_ptr(cur as size_t)
            if(layout_is_overlay(cp.kind)) { layout_place_overlay(state, cur, ctx) }
            else if(layout_is_absolute(cp.spec.position)) { layout_place_absolute(state, cur, ctx) }
            cur = cp.next_sibling
        }
    }
}

func layout_refresh_scrolls(state : &mut LayoutState) {
    var i : size_t = 1u
    while(i < state.result.nodes.size()) {
        var n = state.result.nodes.get_ptr(i)
        unsafe {
            if(n.kind == ViewKind.Scroll && n.id.is_valid()) {
                var off = layout_scroll_offset(state, n.key)
                if(off.x < 0.0f) { off.x = 0.0f }
                if(off.y < 0.0f) { off.y = 0.0f }
                if(off.x > n.scroll_max_x) { off.x = n.scroll_max_x }
                if(off.y > n.scroll_max_y) { off.y = n.scroll_max_y }
                var dx = n.scroll_x - off.x
                var dy = n.scroll_y - off.y
                if(dx != 0.0f || dy != 0.0f) {
                    var cur = n.first_child
                    while(cur != 0u32) {
                        var cp = state.result.nodes.get_ptr(cur as size_t)
                        if(!layout_excluded(cp.kind, cp.spec.position)) { layout_shift_subtree(state, cur, dx, dy) }
                        cur = cp.next_sibling
                    }
                }
                n.scroll_x = off.x; n.scroll_y = off.y
                if(n.scrollbar_visible_y && n.scroll_max_y > 0.0f) {
                    n.scrollbar_thumb_y.y = n.scrollbar_track.y + (off.y / n.scroll_max_y) * (n.scrollbar_track.h - n.scrollbar_thumb_y.h)
                }
                if(n.scrollbar_visible_x && n.scroll_max_x > 0.0f) {
                    n.scrollbar_thumb_x.x = n.scrollbar_track.x + (off.x / n.scroll_max_x) * (n.scrollbar_track.w - n.scrollbar_thumb_x.w)
                }
            }
        }
        i = i + 1u
    }
}

public func layout_run(state : &mut LayoutState, b : &ViewBuilder, ctx : &LayoutContext, theme : &Theme) : LayoutStats {
    var w = layout_finite(state, 0u32, ctx.viewport.width, 30u32)
    var h = layout_finite(state, 0u32, ctx.viewport.height, 31u32)
    if(state.valid && state.last_epoch == b.layout_epoch && state.last_width == w && state.last_height == h) {
        if(state.scroll_epoch != state.last_scroll_epoch) {
            layout_refresh_scrolls(state)
            state.last_scroll_epoch = state.scroll_epoch
        }
        state.result.cached = true
        state.result.pass_count = 0u32
        state.result.visited = 0u32
        return LayoutStats { passes : 0u32, visited : 0u32, warnings : 0u32, cached : true }
    }
    // One style edit on a fixed-box node: if its resolved layout fields did not
    // change, the cached geometry is still valid. This is the fixed-label path.
    if(state.valid && state.last_width == w && state.last_height == h &&
       b.layout_dirty_node != 0u32 && (b.layout_dirty_node as size_t) < state.result.nodes.size()) {
        var old = state.result.nodes.get_ptr(b.layout_dirty_node as size_t)
        unsafe {
            if(old.id.is_valid()) {
                var rs = style_resolve(Style { id : old.style_id }, theme)
                var ns = layout_make_spec(state, b.layout_dirty_node, &rs)
                if(layout_dim_is_px(old.spec.width) && layout_dim_is_px(old.spec.height) && layout_spec_equal(old.spec, ns)) {
                    var vn = b.node_base + (b.layout_dirty_node - 1u32)
                    old.style_id = vn.style_index
                    state.last_epoch = b.layout_epoch
                    state.result.cached = true
                    state.result.pass_count = 0u32
                    state.result.visited = 0u32
                    return LayoutStats { passes : 0u32, visited : 0u32, warnings : state.result.warnings.size() as u32, cached : true }
                }
            }
        }
    }
    state.result.nodes.clear()
    state.result.warnings.clear()
    state.result.cached = false
    state.result.pass_count = 0u32
    state.result.visited = 0u32
    state.result.viewport = Size.new(w, h)
        var i : u32 = 0u32
    while(i <= b.node_count_value) { state.result.nodes.push(layout_empty_node()); i = i + 1u32 }
    if(!b.root.is_valid()) { state.valid = false; return LayoutStats { passes : 1u32, visited : 0u32, warnings : state.result.warnings.size() as u32, cached : false } }
    state.result.root_index = layout_flatten_node(state, b, b.root, 0u32, theme)
    if(state.result.root_index == 0u32) { state.valid = false; return LayoutStats { passes : 1u32, visited : 0u32, warnings : state.result.warnings.size() as u32, cached : false } }
    var root = state.result.root_index
    var measured = layout_intrinsic(state, root, ctx, theme)
    state.result.pass_count = 1u32
    var root_rect = Rect.new(0.0f, 0.0f, w, h)
    var rn = state.result.nodes.get_ptr(root as size_t)
    unsafe {
        rn.rect = root_rect
        layout_place(state, root, ctx)
    }
    state.result.pass_count = 2u32
    state.result.visited = state.result.visited
    state.last_epoch = b.layout_epoch
    state.last_width = w
    state.last_height = h
    state.last_scroll_epoch = state.scroll_epoch
    state.valid = true
    return LayoutStats { passes : 2u32, visited : state.result.visited, warnings : state.result.warnings.size() as u32, cached : false }
}

public func layout_root(st : &LayoutState) : u32 { return st.result.root_index }
public func layout_node_count(st : &LayoutState) : u32 { if(st.result.nodes.size() == 0u) { return 0u32 }; return (st.result.nodes.size() - 1u) as u32 }
public func layout_pass_count(st : &LayoutState) : u32 { return st.result.pass_count }
public func layout_warning_count(st : &LayoutState) : u32 { return st.result.warnings.size() as u32 }
public func layout_cache_hit(st : &LayoutState) : bool { return st.result.cached }
public func layout_node_dirty(st : &LayoutState, id : NodeId) : bool {
    unsafe {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return false }
    return st.result.nodes.get_ptr(id.index() as size_t)[0].dirty

    }}
public func layout_rect(st : &LayoutState, id : NodeId) : Option<Rect> {
    unsafe {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Option.None<Rect>() }
    return Option.Some<Rect>(st.result.nodes.get_ptr(id.index() as size_t)[0].rect)

    }}
public func layout_rect_at(st : &LayoutState, index : u32) : Option<Rect> {
    unsafe {
    if(index == 0u32 || index as size_t >= st.result.nodes.size()) { return Option.None<Rect>() }
    return Option.Some<Rect>(st.result.nodes.get_ptr(index as size_t)[0].rect)

    }}
public func layout_rect_for_key(st : &LayoutState, k : Key) : Option<Rect> {
    unsafe {
    var i : size_t = 1u
    while(i < st.result.nodes.size()) {
        var n = st.result.nodes.get_ptr(i)
        if(n[0].id.is_valid() && n[0].key.equals_key(k)) { return Option.Some<Rect>(n[0].rect) }
        i = i + 1u
    }
    return Option.None<Rect>()

    }}
public func layout_content_width(st : &LayoutState, id : NodeId) : float {
    unsafe {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return 0.0f }
    return st.result.nodes.get_ptr(id.index() as size_t)[0].content_width

    }}
public func layout_content_height(st : &LayoutState, id : NodeId) : float {
    unsafe {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return 0.0f }
    return st.result.nodes.get_ptr(id.index() as size_t)[0].content_height

    }}
public func layout_baseline(st : &LayoutState, id : NodeId) : float {
    unsafe {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return 0.0f }
    return st.result.nodes.get_ptr(id.index() as size_t)[0].baseline

    }}
public func layout_child_at(st : &LayoutState, index : u32, child : u32) : Option<u32> {
    unsafe {
    if(index == 0u32 || index as size_t >= st.result.nodes.size()) { return Option.None<u32>() }
    var cur = st.result.nodes.get_ptr(index as size_t)[0].first_child
    var i = 0u32
    while(cur != 0u32 && i < child) {
        cur = st.result.nodes.get_ptr(cur as size_t)[0].next_sibling
        i = i + 1u32
    }
    if(cur == 0u32) { return Option.None<u32>() }
    return Option.Some<u32>(cur)

    }}
public func layout_scroll_content_size(st : &LayoutState, id : NodeId) : Size {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Size.zero() }
    var n = st.result.nodes.get_ptr(id.index() as size_t)
    return unsafe(Size.new(n.scroll_content_width, n.scroll_content_height))
}
public func layout_scroll_max(st : &LayoutState, id : NodeId) : Point {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Point.zero() }
    var n = st.result.nodes.get_ptr(id.index() as size_t)
    unsafe { return Point { x : n.scroll_max_x, y : n.scroll_max_y } }
}
public func layout_scroll_offset_of(st : &LayoutState, id : NodeId) : Point {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Point.zero() }
    var n = st.result.nodes.get_ptr(id.index() as size_t)
    unsafe { return Point { x : n.scroll_x, y : n.scroll_y } }
}
public func layout_scrollbar_visible_x(st : &LayoutState, id : NodeId) : bool {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return false }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].scrollbar_visible_x)
}
public func layout_scrollbar_visible_y(st : &LayoutState, id : NodeId) : bool {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return false }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].scrollbar_visible_y)
}
public func layout_scrollbar_thumb_x(st : &LayoutState, id : NodeId) : Rect {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Rect.zero() }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].scrollbar_thumb_x)
}
public func layout_scrollbar_thumb_y(st : &LayoutState, id : NodeId) : Rect {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Rect.zero() }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].scrollbar_thumb_y)
}
public func layout_clip(st : &LayoutState, id : NodeId) : Option<Rect> {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Option.None<Rect>() }
    var n = st.result.nodes.get_ptr(id.index() as size_t)
    if(!unsafe(n.has_clip)) { return Option.None<Rect>() }
    return Option.Some<Rect>(unsafe(n.clip))
}
public func layout_scroll_damage(st : &LayoutState, id : NodeId) : Rect {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return Rect.zero() }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].scroll_damage)
}
public func layout_overlay_layer(st : &LayoutState, id : NodeId) : OverlayLayer {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return OverlayLayer.Tooltip }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].overlay_layer)
}
public func layout_z(st : &LayoutState, id : NodeId) : i32 {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return 0i32 }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].z_index)
}
public func layout_is_overlay_node(st : &LayoutState, id : NodeId) : bool {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return false }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].is_overlay)
}
public func layout_line_index(st : &LayoutState, id : NodeId) : u32 {
    if(!id.is_valid() || id.index() as size_t >= st.result.nodes.size()) { return 0u32 }
    return unsafe(st.result.nodes.get_ptr(id.index() as size_t)[0].line_index)
}
public func layout_warning_node(st : &LayoutState, i : u32) : u32 {
    unsafe {
    if(i as size_t >= st.result.warnings.size()) { return 0u32 }
    return st.result.warnings.get_ptr(i as size_t)[0].node

    }}
public func layout_warning_code(st : &LayoutState, i : u32) : u32 {
    unsafe {
    if(i as size_t >= st.result.warnings.size()) { return 0u32 }
    return st.result.warnings.get_ptr(i as size_t)[0].code

    }}

} // namespace ui_internal
