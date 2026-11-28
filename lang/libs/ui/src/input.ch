// P1.11 hit testing, focus traversal and scroll ownership. Internal runtime
// helpers; no input source or platform code lives here.
using std::Option;
using std::Result;
using std::Unit;
using std::vector;

public namespace ui_internal {

@direct_init
public struct HitResult {
    var node : NodeId
    var key : Key
    var bounds : Rect
    var device_bounds : Rect
    var focusable : bool
    var interactive : bool
    var disabled : bool
    var scrollable : bool
    var tab_index : i32
    var order : u32
    var index : u32
}

func record_at(st : &DisplayState, i : size_t) : HitRecord {
    unsafe { return st.hits.get_ptr(i)[0] }
}

func hit_point_in_rect(p : Point, r : Rect) : bool {
    return p.x >= r.x && p.y >= r.y && p.x < r.x2() && p.y < r.y2()
}

func hit_point_in_record(rec : HitRecord, p : Point) : bool {
    if(rec.has_clip && !hit_point_in_rect(p, rec.device_clip)) { return false }
    if(!hit_point_in_rect(p, rec.device_rect)) { return false }
    var t = rec.transform
    var det = t.m[0] * t.m[3] - t.m[1] * t.m[2]
    if(det != 0.0f && det == det) {
        var x = p.x - t.m[4]
        var y = p.y - t.m[5]
        var lx = (t.m[3] * x - t.m[2] * y) / det
        var ly = (0.0f - t.m[1] * x + t.m[0] * y) / det
        return hit_point_in_rect(Point { x : lx, y : ly }, rec.rect)
    }
    return true
}

func node_at(layout : &LayoutState, i : size_t) : LayoutNode {
    unsafe { return layout.result.nodes.get_ptr(i)[0] }
}
func hit_valid(layout : &LayoutState, rec : HitRecord) : bool {
    var i = rec.node.index()
    if(i == 0u32 || i as size_t >= layout.result.nodes.size()) { return false }
    var n = node_at(layout, i as size_t)
    return n.id.index() == rec.node.index() && n.id.generation() == rec.node.generation()
}

func make_hit(rec : HitRecord) : HitResult {
    return HitResult {
        node : rec.node, key : rec.key, bounds : rec.rect,
        device_bounds : rec.device_rect, focusable : rec.focusable,
        interactive : rec.interactive, disabled : rec.disabled,
        scrollable : rec.scrollable, tab_index : rec.tab_index,
        order : rec.order, index : rec.node.index()
    }
}

public func hit_test(st : &DisplayState, layout : &LayoutState, p : Point) : Option<HitResult> {
    var best = -1i32
    var best_order = 0u32
    var i : size_t = 0u
    while(i < st.hits.size()) {
        var rec = record_at(st, i)
        if(hit_valid(layout, rec) && hit_point_in_record(rec, p)) {
            if(best < 0i32 || rec.order >= best_order) { best = i as i32; best_order = rec.order }
        }
        i = i + 1u
    }
    if(best < 0i32) { return Option.None<HitResult>() }
    return Option.Some<HitResult>(make_hit(record_at(st, best as size_t)))
}

public func hit_test_focusable(st : &DisplayState, layout : &LayoutState, p : Point) : Option<HitResult> {
    var best = -1i32
    var best_order = 0u32
    var i : size_t = 0u
    while(i < st.hits.size()) {
        var rec = record_at(st, i)
        if(hit_valid(layout, rec) && rec.focusable && !rec.disabled && hit_point_in_record(rec, p)) {
            if(best < 0i32 || rec.order >= best_order) { best = i as i32; best_order = rec.order }
        }
        i = i + 1u
    }
    if(best < 0i32) { return Option.None<HitResult>() }
    return Option.Some<HitResult>(make_hit(record_at(st, best as size_t)))
}

public func hit_focusable_by_key(st : &DisplayState, k : Key) : Option<HitResult> {
    var i : size_t = 0u
    while(i < st.hits.size()) {
        var r = record_at(st, i)
        if(r.focusable && !r.disabled && r.key.equals_key(k)) { return Option.Some<HitResult>(make_hit(r)) }
        i = i + 1u
    }
    return Option.None<HitResult>()
}

func focus_rank(tab : i32) : i32 {
    if(tab > 0i32) { return 0i32 }
    if(tab == 0i32) { return 1i32 }
    return 2i32
}

func focus_record_before(a : HitRecord, b : HitRecord) : bool {
    var ra = focus_rank(a.tab_index)
    var rb = focus_rank(b.tab_index)
    if(ra != rb) { return ra < rb }
    if(a.tab_index != b.tab_index) {
        if(ra == 0i32) { return a.tab_index < b.tab_index }
        if(ra == 2i32) { return a.tab_index > b.tab_index }
    }
    return a.order < b.order
}

public func focus_next(st : &DisplayState, current : Option<Key>, backwards : bool) : Option<HitResult> {
    var list = vector<HitRecord>()
    var i : size_t = 0u
    while(i < st.hits.size()) {
        var r = record_at(st, i)
        if(r.focusable && !r.disabled) { list.push(r) }
        i = i + 1u
    }
    if(list.size() == 0u) { return Option.None<HitResult>() }
    var a : size_t = 0u
    while(a < list.size()) {
        var b = a + 1u
        while(b < list.size()) {
            var rb = unsafe(list.get_ptr(b)[0])
            var ra = unsafe(list.get_ptr(a)[0])
            if(focus_record_before(rb, ra)) {
                unsafe {
                    list.get_ptr(a)[0] = rb
                    list.get_ptr(b)[0] = ra
                }
            }
            b = b + 1u
        }
        a = a + 1u
    }
    var cur = -1i32
    switch(current) {
        Some(ck) => {
            var j : size_t = 0u
            while(j < list.size()) {
                var rj = unsafe(list.get_ptr(j)[0])
                if(rj.key.equals_key(ck)) { cur = j as i32; j = list.size() }
                j = j + 1u
            }
        }
        None() => { }
    }
    var next : size_t = 0u
    if(cur >= 0i32) {
        if(backwards) { if(cur == 0i32) { next = list.size() - 1u } else { next = (cur - 1i32) as size_t } }
        else { next = ((cur + 1i32) as size_t) % list.size() }
    } else {
        if(backwards) { next = list.size() - 1u } else { next = 0u }
    }
    return Option.Some<HitResult>(make_hit(unsafe(list.get_ptr(next)[0])))
}

public func scroll_ancestor(layout : &LayoutState, start : u32) : Option<u32> {
    var cur = start
    while(cur != 0u32 && (cur as size_t) < layout.result.nodes.size()) {
        var n = node_at(layout, cur as size_t)
        var axis = n.scroll_axis
        if(n.kind == ViewKind.Scroll && (axis == Axis.Vertical || axis == Axis.Horizontal || axis == Axis.Both) && (n.scroll_max_x > 0.0f || n.scroll_max_y > 0.0f)) {
            return Option.Some<u32>(cur)
        }
        cur = n.parent
    }
    return Option.None<u32>()
}

public func scroll_by_hit(layout : &mut LayoutState, hit : &HitResult, dx : float, dy : float) : Option<Key> {
    var a = scroll_ancestor(layout, hit.index)
    if(a is Option.None) { return Option.None<Key>() }
    var Some(ai) = a else return Option.None<Key>()
    var n = node_at(layout, ai as size_t)
    var off = layout_scroll_offset(layout, n.key)
    var nx = off.x + dx
    var ny = off.y + dy
    if(nx < 0.0f) { nx = 0.0f }
    if(ny < 0.0f) { ny = 0.0f }
    if(nx > n.scroll_max_x) { nx = n.scroll_max_x }
    if(ny > n.scroll_max_y) { ny = n.scroll_max_y }
    layout_set_scroll(layout, n.key, Point { x : nx, y : ny })
    return Option.Some<Key>(n.key)
}

public func display_hit_count(st : &DisplayState) : u32 { return st.hits.size() as u32 }

} // namespace ui_internal
