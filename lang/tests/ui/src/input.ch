// P1.11 hit testing, deterministic input dispatch, focus and scroll.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::Unit;
using std::vector;
using ui_internal::HitResult;
using ui_internal::LayoutState;
using ui_internal::DisplayState;
using ui_internal::new_view_builder;
using ui_internal::new_layout_state;
using ui_internal::new_layout_context;
using ui_internal::layout_run;
using ui_internal::layout_rect_for_key;
using ui_internal::display_build;
using ui_internal::display_hit_count;
using ui_internal::hit_test;
using ui_internal::hit_test_focusable;
using ui_internal::hit_focusable_by_key;
using ui_internal::focus_next;
using ui_internal::display_hit_count;

public namespace input_tests {

func fail(name : *char) : bool { unsafe { printf("input FAIL %s\n", name) }; return false }
func mods() : Modifiers { return Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false } }
func shift_mods() : Modifiers { return Modifiers { shift : true, ctrl : false, alt : false, meta : false, caps : false } }
func same_file(a : &path::Path, b : &path::Path) : bool {
    var ra = fs::read(a); var rb = fs::read(b)
    if(ra is Result.Err || rb is Result.Err) { return false }
    var Ok(da) = ra else return false; var Ok(db) = rb else return false
    if(da.size() != db.size()) { return false }
    var i : size_t = 0u
    while(i < da.size()) { if(unsafe(da.get_ptr(i)[0]) != unsafe(db.get_ptr(i)[0])) { return false }; i = i + 1u }
    return true
}
func scene(b : &mut ViewBuilder) {
    var root = ui_internal::view_root(b)
    var canvas = b.column(key("canvas"), Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
    b.child(root, canvas)
    var row = b.row(key("row"), Style.new().width(Dim.Px(360.0f)).height(Dim.Px(80.0f)).gap(Space.SM))
    b.child(canvas, row)
    var a = b.row(key("a"), Style.new().width(Dim.Px(80.0f)).height(Dim.Px(60.0f)).background(ColorToken.SurfaceRaised).radius(Radius.MD)); b.focusable(a, 0i32); b.child(row, a)
    var c = b.row(key("c"), Style.new().width(Dim.Px(80.0f)).height(Dim.Px(60.0f)).background(ColorToken.Danger).radius(Radius.SM)); b.focusable(c, 0i32); b.child(row, c)
    var d = b.row(key("d"), Style.new().width(Dim.Px(80.0f)).height(Dim.Px(60.0f)).background(ColorToken.Success).radius(Radius.LG)); b.focusable(d, 1i32); b.disabled(d, true); b.child(row, d)
    var sc = b.scroll(key("scroll"), Style.new().width(Dim.Px(200.0f)).height(Dim.Px(80.0f)), Axis.Both)
    b.child(canvas, sc)
    var content = b.column(key("content"), Style.new().width(Dim.Px(300.0f)).height(Dim.Px(300.0f)))
    b.child(sc, content)
    var s1 = b.row(key("s1"), Style.new().width(Dim.Px(300.0f)).height(Dim.Px(100.0f)).background(ColorToken.Secondary)); b.focusable(s1, 0i32); b.child(content, s1)
    var s2 = b.row(key("s2"), Style.new().width(Dim.Px(300.0f)).height(Dim.Px(100.0f)).background(ColorToken.Primary)); b.focusable(s2, 0i32); b.child(content, s2)
}
func build_scene(cap : u32, w : float, h : float) : ViewBuilder {
    var b = new_view_builder(cap, w, h); scene(&mut b); return b
}
func build_display(w : float, h : float) : DisplayState {
    var b = build_scene(64u32, w, h); var theme = Theme.light()
    var lc = new_layout_context(ui_internal::empty_text_context(), w, h)
    var ls = new_layout_state(64u32); var _ = layout_run(&mut ls, &b, &lc, &theme)
    var ds = DisplayState.new(512u32); var _n = display_build(&mut ds, &b, &ls, &theme, &lc.text, Size.new(w,h))
    return ds
}
func center_of_key(ls : &LayoutState, k : Key) : Point {
    var r = layout_rect_for_key(ls, k)
    if(r is Option.Some) { var Some(v) = r else return Point.zero(); return Point { x : v.x + v.w * 0.5f, y : v.y + v.h * 0.5f } }
    return Point.zero()
}
func hit_checks() : bool {
    var b = build_scene(64u32, 400.0f, 240.0f); var theme = Theme.light()
    var lc = new_layout_context(ui_internal::empty_text_context(), 400.0f, 240.0f)
    var ls = new_layout_state(64u32); var _ = layout_run(&mut ls, &b, &lc, &theme)
    var ds = DisplayState.new(512u32); var _n = display_build(&mut ds, &b, &ls, &theme, &lc.text, Size.new(400.0f,240.0f))
    if(display_hit_count(&ds) < 5u32) { return fail("hit-records") }
    var pa = center_of_key(&ls, key("a"))
    var h = hit_test(&ds, &ls, pa)
    if(h is Option.None) { return fail("hit-a") }
    var Some(ha) = h else return false
    if(!ha.key.equals_key(key("a"))) { return fail("hit-a-key") }
    var pd = center_of_key(&ls, key("d"))
    var hd = hit_test(&ds, &ls, pd)
    if(hd is Option.None) { return fail("hit-d") }
    var Some(hdv) = hd else return false
    if(!hdv.disabled) { return fail("hit-disabled") }
    var hf = hit_test_focusable(&ds, &ls, pd)
    if(hf is Option.Some) { return fail("focus-disabled") }
    if(hit_focusable_by_key(&ds, key("missing")).is_some()) { return fail("stale-key") }
    // Transform: a translated copy is not hit at its old center through the
    // same display; use a fresh transformed scene.
    var bt = new_view_builder(32u32, 200.0f, 160.0f); var root = ui_internal::view_root(&bt)
    var tx = bt.row(key("tx"), Style.new().width(Dim.Px(80.0f)).height(Dim.Px(60.0f)).background(ColorToken.Primary)); bt.focusable(tx, 0i32); bt.child(root, tx); bt.transform(tx, Transform2D.translate(40.0f, 30.0f))
    var lt = new_layout_state(32u32); var lct = new_layout_context(ui_internal::empty_text_context(),200.0f,160.0f); var _2=layout_run(&mut lt,&bt,&lct,&theme)
    var dst = DisplayState.new(64u32); var _3=display_build(&mut dst,&bt,&lt,&theme,&lct.text,Size.new(200.0f,160.0f))
    var ht = hit_test(&dst,&lt,Point { x : 60.0f, y : 50.0f })
    if(ht is Option.None) { return fail("hit-transform") }
    var bo = new_view_builder(32u32, 200.0f, 200.0f); var oroot = ui_internal::view_root(&bo)
    var base = bo.row(key("base"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)).background(ColorToken.Primary)); bo.focusable(base,0i32); bo.child(oroot,base)
    var ov = bo.overlay(key("ov"), OverlayLayer.Dialog, Option.Some<NodeId>(oroot))
    bo.style(ov, Style.new().width(Dim.Px(80.0f)).height(Dim.Px(60.0f)).background(ColorToken.Danger)); bo.focusable(ov,0i32); bo.child(oroot,ov)
    var lo = new_layout_state(32u32); var lco = new_layout_context(ui_internal::empty_text_context(),200.0f,200.0f); var _4=layout_run(&mut lo,&bo,&lco,&theme)
    var dso = DisplayState.new(64u32); var _5=display_build(&mut dso,&bo,&lo,&theme,&lco.text,Size.new(200.0f,200.0f))
    var pclip = hit_test(&ds,&ls,Point { x : 250.0f, y : 120.0f })
    switch(pclip) {
        Some(hc) => { if(hc.key.equals_key(key("s1"))) { return fail("hit-clip") } }
        None() => { }
    }
    var ho = hit_test(&dso,&lo,Point { x : 10.0f, y : 10.0f })
    if(ho is Option.None) { return fail("hit-overlay") }
    var Some(hov)=ho else return false
    if(!hov.key.equals_key(key("ov"))) { return fail("hit-overlay-key") }
    return true
}
func focus_checks() : bool {
    var b = build_scene(64u32, 400.0f, 240.0f); var theme=Theme.light(); var lc=new_layout_context(ui_internal::empty_text_context(),400.0f,240.0f); var ls=new_layout_state(64u32); var _=layout_run(&mut ls,&b,&lc,&theme)
    var ds=DisplayState.new(512u32); var _n=display_build(&mut ds,&b,&ls,&theme,&lc.text,Size.new(400.0f,240.0f))
    var f = focus_next(&ds, Option.None<Key>(), false)
    if(f is Option.None) { return fail("focus-first") }
    var Some(first) = f else return false
    if(!first.key.equals_key(key("a"))) { return fail("focus-order") }
    var s = focus_next(&ds, Option.Some<Key>(key("a")), false)
    if(s is Option.None) { return fail("focus-next") }
    var Some(second) = s else return false
    if(!second.key.equals_key(key("c"))) { return fail("focus-skip-disabled") }
    var back = focus_next(&ds, Option.Some<Key>(key("a")), true)
    if(back is Option.None) { return fail("focus-back") }
    var Some(last) = back else return false
    if(last.key.equals_key(key("a"))) { return fail("focus-back-wrap") }
    return true
}
func hit_10k_time() : bool {
    var b = new_view_builder(10016u32, 10000.0f, 20.0f); var root = ui_internal::view_root(&b)
    var row = b.row(key("row"), Style.new().width(Dim.Px(10000.0f)).height(Dim.Px(20.0f))); b.child(root,row)
    var i=0u32
    while(i<10000u32) { var n=b.row(key_of("n",i as u64),Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f)).background(ColorToken.Primary)); b.focusable(n,0i32); b.child(row,n); i=i+1u32 }
    var theme=Theme.light(); var lc=new_layout_context(ui_internal::empty_text_context(),10000.0f,20.0f); var ls=new_layout_state(10016u32); var _=layout_run(&mut ls,&b,&lc,&theme)
    var ds=DisplayState.new(10016u32); var _n=display_build(&mut ds,&b,&ls,&theme,&lc.text,Size.new(10000.0f,20.0f))
    var times : [5]i64
    var t=0
    while(t<5) { var s=std::chrono::Instant.now(); var j=0u32; while(j<100u32){var _h=hit_test(&ds,&ls,Point { x : 5000.5f, y : 10.5f }); j=j+1u32}; times[t]=s.elapsed().as_micros()/100i64; t=t+1 }
    var a=0; while(a<5){var b2=a+1; while(b2<5){if(times[b2]<times[a]){var x=times[a];times[a]=times[b2];times[b2]=x};b2=b2+1};a=a+1}
    unsafe { printf("input hit10k_us p50=%ld p95=%ld\n", times[2], times[4]) }
    if(is_llvm() && times[2] > 500i64) { return false }
    return true
}
public struct InputApp {
    var updates : u32
    var last_kind : EventKind
    var last_key : Key
    var last_focus : Option<Key>
    var last_scroll : Point
    var last_repeat : bool
    var last_shift : bool
    var last_code : KeyCode
}
impl Application for InputApp {
    public func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId,UiError> { scene(b); return Result.Ok<NodeId,UiError>(ui_internal::view_root(b)) }
    public func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit,UiError> {
        self.updates = self.updates + 1u32
        self.last_kind = ev.kind
        self.last_key = ev.key
        self.last_focus = ctx.focused_key()
        self.last_scroll = ctx.scroll_offset(ev.key)
        return Result.Ok<Unit,UiError>(Unit{})
    }
    public func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction { return CloseAction.Allow }
}
func options(w : i32, h : i32, frames : u32) : Options {
    var o = Options.make(); o.width=w; o.height=h; o.max_frames=frames; o.fixed_time=true; o.headless=true; o.theme=ThemeMode.Light; o.scale=1.0f
    o.font_file = "lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf"; return o
}
func click_checks() : bool {
    headless_input_reset()
    headless_input_pointer(EventKind.PointerMove, 50.0f, 30.0f, 0u8, mods())
    headless_input_pointer(EventKind.Press, 50.0f, 30.0f, 0u8, mods())
    headless_input_pointer(EventKind.Release, 50.0f, 30.0f, 0u8, mods())
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r = run_headless(&mut app, options(400,240,1u32))
    if(r is Result.Err) { return fail("click-run") }
    if(app.last_kind != EventKind.Click) { return fail("click-kind") }
    if(!app.last_key.equals_key(key("a"))) { return fail("click-key") }
    if(app.last_focus is Option.None) { return fail("click-focus") }
    var Some(fk) = app.last_focus else return false
    if(!fk.equals_key(key("a"))) { return fail("click-focus-key") }
    return true
}
func tab_checks() : bool {
    headless_input_reset()
    headless_input_key(EventKind.KeyDown, KeyCode.Tab, mods(), false, "")
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r = run_headless(&mut app, options(400,240,2u32))
    if(r is Result.Err) { return fail("tab-run") }
    if(app.updates == 0u32) { return fail("tab-updates") }
    if(app.last_focus is Option.None) { return fail("tab-focus") }
    var Some(tf) = app.last_focus else return false
    if(!tf.equals_key(key("a"))) { return fail("tab-focus-key") }
    headless_input_reset()
    headless_input_key(EventKind.KeyDown, KeyCode.Tab, shift_mods(), false, "")
    var app2 = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r2 = run_headless(&mut app2, options(400,240,2u32))
    if(r2 is Result.Err) { return fail("shift-tab-run") }
    if(app2.last_focus is Option.None) { return fail("shift-tab-focus") }
    var Some(tb) = app2.last_focus else return false
    if(tb.equals_key(key("a"))) { return fail("shift-tab-wrap") }
    return true
}
func wheel_checks() : bool {
    headless_input_reset()
    headless_input_wheel(100.0f, 120.0f, 20.0f, 30.0f, mods())
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r = run_headless(&mut app, options(400,240,1u32))
    if(r is Result.Err) { return fail("wheel-run") }
    if(app.last_kind != EventKind.Scroll) { return fail("wheel-kind") }
    if(app.last_scroll.y <= 0.0f) { return fail("wheel-offset") }
    return true
}
func double_checks() : bool {
    headless_input_reset()
    headless_input_pointer_at(EventKind.Press, 50.0f, 30.0f, 0u8, mods(), 100.0)
    headless_input_pointer_at(EventKind.Release, 50.0f, 30.0f, 0u8, mods(), 120.0)
    headless_input_pointer_at(EventKind.Press, 50.0f, 30.0f, 0u8, mods(), 200.0)
    headless_input_pointer_at(EventKind.Release, 50.0f, 30.0f, 0u8, mods(), 220.0)
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r = run_headless(&mut app, options(400,240,1u32))
    if(r is Result.Err) { return fail("double-run") }
    if(app.last_kind != EventKind.DoubleClick) { return fail("double-kind") }
    return true
}
func screenshot_checks() : bool {
    headless_input_reset()
    headless_input_pointer(EventKind.PointerMove, 50.0f, 30.0f, 0u8, mods())
    headless_input_pointer(EventKind.Press, 50.0f, 30.0f, 0u8, mods())
    headless_input_pointer(EventKind.Release, 50.0f, 30.0f, 0u8, mods())
    headless_input_pointer(EventKind.PointerMove, 150.0f, 30.0f, 0u8, mods())
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var expected = path::PathBuf.from("lang/tests/ui/fixtures/input/focus_hover.png")
    var tmp = path::PathBuf.from("/tmp/ui_input_focus.png")
    var update=false; var e=environment::get("CHEMICAL_UI_UPDATE_GOLDENS"); if(e is Option.Some){var Some(v)=e else return false; update=v.to_view().equals_with_len("1",1u)}
    var out = tmp
    if(update) { out = expected }
    var s = run_headless_shot(&mut app, options(400,240,5u32), out.as_view())
    if(s is Result.Err) { return fail("png-run") }
    if(update) { return true }
    if(!same_file(&tmp.as_path(), &expected.as_path())) { return fail("png-golden") }
    return true
}
func escape_checks() : bool {
    headless_input_reset()
    headless_input_key(EventKind.KeyDown, KeyCode.Tab, mods(), false, "")
    headless_input_key(EventKind.KeyDown, KeyCode.Escape, mods(), false, "")
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r = run_headless(&mut app, options(400,240,3u32))
    if(r is Result.Err) { return fail("escape-run") }
    if(app.last_focus is Option.Some) { return fail("escape-focus") }
    return true
}
func scroll_10k_time() : bool {
    var b = new_view_builder(10032u32, 100.0f, 100.0f); var root=ui_internal::view_root(&b)
    var sc=b.scroll(key("sc"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)), Axis.Both); b.child(root,sc)
    var content=b.column(key("content"), Style.new().width(Dim.Px(1000.0f)).height(Dim.Px(10000.0f))); b.child(sc,content)
    var i=0u32
    while(i<10000u32) { var n=b.row(key_of("t",i as u64),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(1.0f)).background(ColorToken.Primary)); b.child(content,n); i=i+1u32 }
    var theme=Theme.light(); var lc=new_layout_context(ui_internal::empty_text_context(),100.0f,100.0f); var ls=new_layout_state(10032u32); var _=layout_run(&mut ls,&b,&lc,&theme)
    var ds=DisplayState.new(12000u32); var _n=display_build(&mut ds,&b,&ls,&theme,&lc.text,Size.new(100.0f,100.0f))
    var h=hit_test(&ds,&ls,Point { x : 50.0f, y : 50.0f })
    if(h is Option.None) { return false }
    var Some(hv)=h else return false
    var times:[5]i64; var t=0
    while(t<5) { var s=std::chrono::Instant.now(); var j=0u32; while(j<100u32){var _g=ui_internal::scroll_by_hit(&mut ls,&hv,0.5f,0.5f);j=j+1u32}; times[t]=s.elapsed().as_micros()/100i64; t=t+1 }
    var a=0; while(a<5){var b2=a+1; while(b2<5){if(times[b2]<times[a]){var x=times[a];times[a]=times[b2];times[b2]=x};b2=b2+1};a=a+1}
    unsafe { printf("input scroll10k_us p50=%ld p95=%ld\n", times[2], times[4]) }
    if(is_llvm() && times[2] > 500i64) { return false }
    return true
}
func event_order_checks() : bool {
    headless_input_reset()
    headless_input_key(EventKind.KeyDown, KeyCode.A, mods(), false, "")
    headless_input_key(EventKind.KeyDown, KeyCode.A, mods(), true, "")
    headless_input_text("x")
    var qe = headless_queue_at(1u)
    var qkp = headless_event_key_payload(&qe)
    if(!qkp.repeat) { return fail("queue-repeat") }
    var app = InputApp { updates:0u32, last_kind:EventKind.Frame, last_key:zeroed<Key>(), last_focus:Option.None<Key>(), last_scroll:Point.zero(), last_repeat:false, last_shift:false, last_code:KeyCode.Unknown }
    var r = run_headless(&mut app, options(400,240,1u32))
    if(r is Result.Err) { return fail("order-run") }
    if(app.updates < 3u32) { return fail("order-count") }
    unsafe { printf("input event_max_us=%.3f\n", headless_event_max_us()) }
    if(is_release() && headless_event_max_us() > 1000.0) { return fail("event-time") }
    return true
}
func run_input_tests() : bool {
    headless_input_reset()
    headless_event_reset_timing()
    if(!hit_checks()) { return false }
    if(!focus_checks()) { return false }
    if(!click_checks()) { return false }
    if(!tab_checks()) { return false }
    if(!wheel_checks()) { return false }
    if(!double_checks()) { return false }
    if(!screenshot_checks()) { return false }
    if(!escape_checks()) { return false }
    if(!event_order_checks()) { return false }
    if(is_release()) { if(!hit_10k_time()) { return false }; if(!scroll_10k_time()) { return false } }
    return true
}
func is_release() : bool { var e=environment::get("CHEMICAL_UI_RELEASE"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("1",1u)}; return false }
func is_llvm() : bool { var e=environment::get("CHEMICAL_UI_TEST_BACKEND"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("llvm",4u)}; return false }
}
