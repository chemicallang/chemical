// P1.6 layout v1 structural, fixture, property and release checks.
using std::Option;
using std::Result;
using std::string_view;
using std::vector;
using ui_internal::LayoutState;
using ui_internal::LayoutContext;
using ui_internal::layout_root;
using ui_internal::layout_child_at;
using ui_internal::new_view_builder;
using ui_internal::reset_view_builder;
using ui_internal::view_root;
using ui_internal::view_child_at;
using ui_internal::new_layout_state;
using ui_internal::new_layout_context;
using ui_internal::layout_run;
using ui_internal::layout_rect;
using ui_internal::layout_rect_for_key;
using ui_internal::layout_baseline;
using ui_internal::layout_pass_count;
using ui_internal::layout_warning_count;
using ui_internal::layout_cache_hit;
using ui_internal::layout_node_dirty;
using ui_internal::empty_text_context;
using ui_internal::text_context_from_font;

public namespace layout_tests {

func close(a : float, b : float) : bool {
    var d = a - b
    if(d < 0.0f) { d = 0.0f - d }
    return d <= 0.5f
}
func fail(name : *char) : bool {
    unsafe { printf("layout FAIL %s\n", name) }
    return false
}
func rect_eq(r : Rect, x : float, y : float, w : float, h : float) : bool {
    return close(r.x, x) && close(r.y, y) && close(r.w, w) && close(r.h, h)
}
func get_rect(st : &LayoutState, id : NodeId) : Rect {
    var r = layout_rect(st, id)
    if(r is Option.Some) { var Some(v) = r else return Rect.zero(); return v }
    return Rect.zero()
}
func fixed_box(b : &mut ViewBuilder, k : Key, w : float, h : float, parent : NodeId) : NodeId {
    var n = b.row(k, Style.new().width(Dim.Px(w)).height(Dim.Px(h)))
    b.child(parent, n)
    return n
}
func basic_fixtures() : bool {
    var b = new_view_builder(64u32, 200.0f, 100.0f)
    var root = view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(200.0f)).height(Dim.Px(20.0f)))
    b.child(root, row)
    var a = fixed_box(&mut b, key("a"), 10.0f, 10.0f, row)
    var c = fixed_box(&mut b, key("c"), 20.0f, 10.0f, row)
    var theme = Theme.light()
    var ctx = new_layout_context(empty_text_context(), 200.0f, 100.0f)
    var st = new_layout_state(64u32)
    var stats = layout_run(&mut st, &b, &ctx, &theme)
    if(stats.passes == 0u32 || stats.passes > 3u32) { return fail("basic-pass") }
    var r0 = get_rect(&st, a)
    var r1 = get_rect(&st, c)
    if(!rect_eq(r0, 0.0f, 0.0f, 10.0f, 10.0f) || !rect_eq(r1, 10.0f, 0.0f, 20.0f, 10.0f)) { return fail("basic-row") }
    // Warm no-change: 0 passes and same rects.
    var warm = layout_run(&mut st, &b, &ctx, &theme)
    if(warm.passes != 0u32 || !layout_cache_hit(&st) || !rect_eq(get_rect(&st, c), 10.0f, 0.0f, 20.0f, 10.0f)) { return fail("warm-cache") }
    if(layout_node_dirty(&st, a)) { return fail("warm-dirty") }
    return true
}
func grow_shrink_padding() : bool {
    var b = new_view_builder(64u32, 200.0f, 100.0f)
    var root = view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(200.0f)).height(Dim.Px(20.0f)).gap(Space.SM).padding_all(Space.SM))
    b.child(root, row)
    var a = b.row(key("a"), Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f)).grow(1.0f)); b.child(row, a)
    var c = b.row(key("c"), Style.new().width(Dim.Px(20.0f)).height(Dim.Px(10.0f)).grow(1.0f)); b.child(row, c)
    var theme = Theme.light()
    var ctx = new_layout_context(empty_text_context(), 200.0f, 100.0f)
    var st = new_layout_state(64u32)
    var _ = layout_run(&mut st, &b, &ctx, &theme)
    // content width 200-16=184, gap 8, base 30, leftover 146/2=73
    if(!rect_eq(get_rect(&st,a), 8.0f, 8.0f, 83.0f, 10.0f) || !rect_eq(get_rect(&st,c), 99.0f, 8.0f, 93.0f, 10.0f)) { return fail("grow-space") }
    var b2 = new_view_builder(64u32, 100.0f, 100.0f)
    var root2 = view_root(&b2)
    var col = b2.column(key("col"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(30.0f)))
    b2.child(root2, col)
    var x = b2.row(key("x"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(30.0f)).shrink(1.0f)); b2.child(col, x)
    var y = b2.row(key("y"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(30.0f)).shrink(1.0f)); b2.child(col, y)
    var theme2 = Theme.light()
    var ctx2 = new_layout_context(empty_text_context(), 100.0f, 100.0f)
    var st2 = new_layout_state(64u32)
    var _2 = layout_run(&mut st2, &b2, &ctx2, &theme2)
    if(!rect_eq(get_rect(&st2,x), 0.0f, 0.0f, 100.0f, 15.0f) || !rect_eq(get_rect(&st2,y), 0.0f, 15.0f, 100.0f, 15.0f)) { return fail("shrink") }
    return true
}
func percent_fill_align() : bool {
    var b = new_view_builder(64u32, 200.0f, 100.0f)
    var root = view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(40.0f)).align_items(Align.Center))
    b.child(root, row)
    var pct = b.row(key("pct"), Style.new().width(Dim.Percent(0.5f)).height(Dim.Px(10.0f))); b.child(row, pct)
    var fill = b.row(key("fill"), Style.new().width(Dim.Fill()).height(Dim.Px(10.0f))); b.child(row, fill)
    var theme = Theme.light()
    var ctx = new_layout_context(empty_text_context(), 200.0f, 100.0f)
    var st = new_layout_state(64u32)
    var _ = layout_run(&mut st, &b, &ctx, &theme)
    if(!rect_eq(get_rect(&st,pct), 0.0f, 15.0f, 50.0f, 10.0f)) { return fail("percent") }
    if(!rect_eq(get_rect(&st,fill), 50.0f, 15.0f, 50.0f, 10.0f)) { return fail("fill") }
    var bj = new_view_builder(64u32, 100.0f, 100.0f); var rj = view_root(&bj)
    var jr = bj.row(key("jr"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(20.0f)).justify(Justify.SpaceBetween)); bj.child(rj, jr)
    var j1 = bj.row(key("j1"), Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f))); bj.child(jr,j1)
    var j2 = bj.row(key("j2"), Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f))); bj.child(jr,j2)
    var tj = Theme.light(); var cj = new_layout_context(empty_text_context(),100.0f,100.0f); var sj = new_layout_state(64u32)
    var _j = layout_run(&mut sj,&bj,&cj,&tj)
    if(!rect_eq(get_rect(&sj,j1),0.0f,0.0f,10.0f,10.0f) || !rect_eq(get_rect(&sj,j2),90.0f,0.0f,10.0f,10.0f)) { return fail("justify") }
    return true
}
func min_max_and_stack() : bool {
    var b = new_view_builder(64u32, 100.0f, 100.0f)
    var root = view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(20.0f))); b.child(root, row)
    var a = b.row(key("a"), Style.new().width(Dim.Px(30.0f)).max_width(20.0f).height(Dim.Px(10.0f))); b.child(row, a)
    var c = b.row(key("c"), Style.new().width(Dim.Px(5.0f)).min_width(10.0f).height(Dim.Px(10.0f))); b.child(row, c)
    var theme = Theme.light(); var ctx = new_layout_context(empty_text_context(), 100.0f, 100.0f)
    var st = new_layout_state(64u32)
    var _ = layout_run(&mut st, &b, &ctx, &theme)
    if(!rect_eq(get_rect(&st,a), 0.0f, 0.0f, 20.0f, 10.0f) || !rect_eq(get_rect(&st,c), 20.0f, 0.0f, 10.0f, 10.0f)) { return fail("min-max") }
    var b2 = new_view_builder(64u32, 100.0f, 100.0f); var r2 = view_root(&b2)
    var stk = b2.stack(key("stack"), Style.new().width(Dim.Px(50.0f)).height(Dim.Px(50.0f))); b2.child(r2, stk)
    var one = b2.row(key("one"), Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f))); b2.child(stk, one)
    var two = b2.row(key("two"), Style.new().width(Dim.Px(20.0f)).height(Dim.Px(20.0f))); b2.child(stk, two)
    var t2 = Theme.light(); var c2 = new_layout_context(empty_text_context(), 100.0f, 100.0f); var s2 = new_layout_state(64u32)
    var _2 = layout_run(&mut s2, &b2, &c2, &t2)
    if(!rect_eq(get_rect(&s2,one), 0.0f, 0.0f, 10.0f, 10.0f) || !rect_eq(get_rect(&s2,two), 0.0f, 0.0f, 20.0f, 20.0f)) { return fail("stack") }
    return true
}
func warning_clamp() : bool {
    var b = new_view_builder(16u32, 100.0f, 100.0f)
    var root = view_root(&b)
    var bad = b.row(key("bad"), Style.new().width(Dim.Px(1.0f / 0.0f)).height(Dim.Px(-1.0f)))
    b.child(root, bad)
    var theme = Theme.light()
    var ctx = new_layout_context(empty_text_context(), 100.0f, 100.0f)
    var st = new_layout_state(16u32)
    var _ = layout_run(&mut st, &b, &ctx, &theme)
    if(layout_warning_count(&st) == 0u32) { return fail("warnings") }
    var rr = get_rect(&st, bad)
    if(!close(rr.w, 0.0f) || !close(rr.h, 0.0f)) { return fail("clamp") }
    return true
}
func text_and_absolute() : bool {
    var p = path::PathBuf.from("lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf")
    var loaded = font::Font.from_path(&p.as_path())
    if(loaded is Result.Err) { return fail("layout-font") }
    var Ok(f) = loaded else return false
    var theme = Theme.light(); var ctx = new_layout_context(text_context_from_font(f), 100.0f, 100.0f)
    var b = new_view_builder(64u32, 100.0f, 100.0f); var root = view_root(&b)
    var t = b.text(key("t"), "A", Style.new().text(TextRole.Body)); b.child(root, t)
    var st = new_layout_state(64u32)
    var _ = layout_run(&mut st, &b, &ctx, &theme)
    var tr = get_rect(&st, t)
    if(!rect_eq(tr, 0.0f, 0.0f, 9.337891f, 20.0f)) { return fail("text-intrinsic") }
    if(!close(layout_baseline(&st, t), 13.804199f)) { return fail("text-baseline") }
    // Baseline alignment uses the same absolute baseline for both text runs.
    var b2 = new_view_builder(64u32, 100.0f, 100.0f); var r2 = view_root(&b2)
    var row = b2.row(key("row"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)).align_items(Align.Baseline)); b2.child(r2, row)
    var t1 = b2.text(key("t1"), "A", Style.new().text(TextRole.Body)); b2.child(row, t1)
    var t2 = b2.text(key("t2"), "B", Style.new().text(TextRole.H1)); b2.child(row, t2)
    var theme2 = Theme.light(); var ctx2 = new_layout_context(empty_text_context(), 100.0f, 100.0f)
    // Use the same font context by loading a second context; the first was moved.
    var p2 = path::PathBuf.from("lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf")
    var l2 = font::Font.from_path(&p2.as_path())
    if(l2 is Result.Err) { return fail("layout-font2") }
    var Ok(f2) = l2 else return false
    var ctx2b = new_layout_context(text_context_from_font(f2), 100.0f, 100.0f)
    var st2 = new_layout_state(64u32)
    var _2 = layout_run(&mut st2, &b2, &ctx2b, &theme2)
    if(!close(layout_baseline(&st2,t1), layout_baseline(&st2,t2))) { return fail("baseline-align") }
    // Absolute child uses parent content origin plus insets.
    var b3 = new_view_builder(64u32, 100.0f, 100.0f); var r3 = view_root(&b3)
    var box = b3.row(key("box"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f))); b3.child(r3, box)
    var abs = b3.row(key("abs"), Style.new().width(Dim.Px(20.0f)).height(Dim.Px(10.0f)).position(Position.Absolute(Dim.Px(10.0f), Dim.Auto(), Dim.Auto(), Dim.Px(20.0f)))); b3.child(box, abs)
    var theme3 = Theme.light(); var ctx3 = new_layout_context(empty_text_context(), 100.0f, 100.0f)
    var st3 = new_layout_state(64u32)
    var _3 = layout_run(&mut st3, &b3, &ctx3, &theme3)
    if(!rect_eq(get_rect(&st3,abs), 20.0f, 10.0f, 20.0f, 10.0f)) { return fail("absolute") }
    return true
}
func property_fixed_rows() : bool {
    var b = new_view_builder(64u32, 200.0f, 100.0f)
    var st = new_layout_state(64u32)
    var theme = Theme.light(); var ctx = new_layout_context(empty_text_context(), 200.0f, 100.0f)
    var rng = Rng { state : 0x123456789ABCDEFu64 }
    var i = 0u32
    while(i < 1000u32) {
        reset_view_builder(&mut b)
        var root = view_root(&b)
        var p = (rng.next() % 8u64) as float
        var g = (rng.next() % 6u64) as float
        var is_row = (rng.next() % 2u64) == 0u64
        var container : NodeId
        if(is_row) { container = b.row(key("box"), Style.new().width(Dim.Px(200.0f)).height(Dim.Px(100.0f)).padding(Insets.all(p))) }
        else { container = b.column(key("box"), Style.new().width(Dim.Px(200.0f)).height(Dim.Px(100.0f)).padding(Insets.all(p))) }
        b.child(root, container)
        var kids = vector<NodeId>()
        var n = (rng.next() % 5u64 + 1u64) as u32
        var widths = vector<float>()
        var k = 0u32
        while(k < n) {
            var w = (rng.next() % 20u64 + 1u64) as float
            widths.push(w)
            var c = b.row(key_of("p", k as u64), Style.new().width(Dim.Px(w)).height(Dim.Px(10.0f)))
            b.child(container, c); kids.push(c)
            k = k + 1u32
        }
        var _ = layout_run(&mut st, &b, &ctx, &theme)
        var x = p
        var y = p
        k = 0u32
        while(k < n) {
            var rr = get_rect(&st, unsafe(kids.get_ptr(k)[0]))
            var w = unsafe(widths.get_ptr(k)[0])
            if(is_row) {
                if(!close(rr.x, x) || !close(rr.y, p) || !close(rr.w, w)) { return fail("property-row") }
                x = x + w
            } else {
                if(!close(rr.x, p) || !close(rr.y, y) || !close(rr.w, w)) { return fail("property-column") }
                y = y + 10.0f
            }
            k = k + 1u32
        }
        i = i + 1u32
    }
    return true
}
struct Rng { var state : u64
    func next(&mut self) : u64 { var x=self.state; x=x^(x<<13u64); x=x^(x>>7u64); x=x^(x<<17u64); self.state=x; return x }
}
func run_structural() : bool {
    if(!basic_fixtures()) { return false }
    if(!grow_shrink_padding()) { return false }
    if(!percent_fill_align()) { return false }
    if(!warning_clamp()) { return false }
    if(!min_max_and_stack()) { return false }
    if(!text_and_absolute()) { return false }
    if(!property_fixed_rows()) { return false }
    return true
}
func is_llvm() : bool {
    var e = environment::get("CHEMICAL_UI_TEST_BACKEND")
    if(e is Option.Some) { var Some(v)=e else return false; return v.to_view().equals_with_len("llvm",4u) }
    return false
}
func is_release() : bool {
    var e = environment::get("CHEMICAL_UI_RELEASE")
    if(e is Option.Some) { var Some(v)=e else return false; return v.to_view().equals_with_len("1",1u) }
    return false
}
func build_fixed(count : u32) : ViewBuilder {
    var b = new_view_builder(count + 64u32, 200000.0f, 100.0f)
    var root = view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(200000.0f)).height(Dim.Px(20.0f))); b.child(root,row)
    var i=0u32
    while(i<count) { var n=b.row(key_of("b",i as u64), Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f))); b.child(row,n); i=i+1u32 }
    return b
}
func build_mixed(count : u32) : ViewBuilder {
    var b = new_view_builder(count + 64u32, 200000.0f, 100.0f)
    var root = view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(200000.0f)).height(Dim.Px(20.0f))); b.child(root,row)
    var i=0u32
    while(i<count) {
        if(i % 5u == 0u) { var t=b.text(key_of("t",i as u64), "A", Style.new().text(TextRole.Body)); b.child(row,t) }
        else { var n=b.row(key_of("b",i as u64), Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f))); b.child(row,n) }
        i=i+1u32
    }
    return b
}
func time_layout(b : &ViewBuilder, ctx : &LayoutContext, theme : &Theme, capacity : u32) : i64 {
    var st = new_layout_state(capacity)
    var start = std::chrono::Instant.now()
    var _ = layout_run(&mut st, b, ctx, theme)
    return start.elapsed().as_micros()
}
func sorted5(a : *mut i64) {
    var i=0; while(i<5){ var j=i+1; while(j<5){ if(unsafe(a[j]) < unsafe(a[i])) { var t=unsafe(a[i]); unsafe{ a[i]=unsafe(a[j]); a[j]=t } }; j=j+1 }; i=i+1 }
}
func run_perf() : bool {
    var theme = Theme.light()
    var empty_ctx = new_layout_context(empty_text_context(), 200000.0f,100.0f)
    var simple = build_fixed(10000u32)
    var st : [5]i64
    var i=0; while(i<5){ st[i]=time_layout(&simple,&empty_ctx,&theme,10050u32); i=i+1 }
    sorted5(&raw mut st[0])
    unsafe { printf("layout simple_us p50=%ld p95=%ld\n", st[2], st[4]) }
    // Warm no-change.
    var warm_state = new_layout_state(10050u32); var _=layout_run(&mut warm_state,&simple,&empty_ctx,&theme); var w=layout_run(&mut warm_state,&simple,&empty_ctx,&theme)
    unsafe { printf("layout warm_passes=%u cached=%d\n", w.passes, layout_cache_hit(&warm_state) as int) }
    var mixed = build_mixed(10000u32)
    var p = path::PathBuf.from("lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf")
    var loaded = font::Font.from_path(&p.as_path())
    if(loaded is Result.Err) { return fail("perf-font") }
    var Ok(f)=loaded else return false
    var tctx = new_layout_context(text_context_from_font(f),200000.0f,100.0f)
    var mt : [5]i64
    i=0; while(i<5){ mt[i]=time_layout(&mixed,&tctx,&theme,10050u32); i=i+1 }
    sorted5(&raw mut mt[0])
    unsafe { printf("layout mixed_us p50=%ld p95=%ld\n", mt[2], mt[4]) }
    var wide = build_fixed(100000u32)
    var wt : [5]i64
    i=0; while(i<5){ wt[i]=time_layout(&wide,&empty_ctx,&theme,100050u32); i=i+1 }
    sorted5(&raw mut wt[0])
    unsafe { printf("layout wide_us p50=%ld p95=%ld\n", wt[2], wt[4]) }
    // One fixed label/style change in the 10k tree (full layout currently).
    var first = view_root(&simple)
    var row_id = view_child_at(&simple, first, 0u32)
    if(row_id is Option.None) { return fail("one-row") }
    var Some(rid)=row_id else return false
    var child_id = view_child_at(&simple, rid, 0u32)
    if(child_id is Option.None) { return fail("one-child") }
    var Some(cid)=child_id else return false
    var one_times : [5]i64
    var oi=0
    while(oi<5) {
        simple.style(cid, Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f)).opacity(0.5f))
        var one_start = std::chrono::Instant.now()
        var one = layout_run(&mut warm_state, &simple, &empty_ctx, &theme)
        one_times[oi] = one_start.elapsed().as_micros()
        if(one.passes != 0u32 || !layout_cache_hit(&warm_state)) { return fail("one-cache") }
        oi=oi+1
    }
    sorted5(&raw mut one_times[0])
    unsafe { printf("layout one_change_us p50=%ld p95=%ld\n", one_times[2], one_times[4]) }
    if(is_llvm()) {
        if(st[2] > 2500i64 || st[4] > 4000i64) { return fail("simple-gate") }
        if(mt[2] > 6000i64 || mt[4] > 9000i64) { return fail("mixed-gate") }
        if(wt[2] > 40000i64 || wt[4] > 60000i64) { return fail("wide-gate") }
        if(one_times[4] > 100i64 || w.passes != 0u32 || !layout_cache_hit(&warm_state)) { return fail("one-gate") }
    }
    return true
}
public func run_layout() : bool {
    if(!run_structural()) { return false }
    if(is_release()) { if(!run_perf()) { return false } }
    return true
}
}
