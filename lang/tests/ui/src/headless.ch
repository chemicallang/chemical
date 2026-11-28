// P1.10 headless application loop, deterministic PNG fixtures, and error paths.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::Unit;
using std::vector;

public namespace headless_tests {

func fail(name : *char) : bool {
    unsafe { printf("headless FAIL %s\n", name) }
    return false
}
func asset_font() : string_view {
    return "lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf"
}
func options_for(w : i32, h : i32) : Options {
    var o = Options.make()
    o.width = w
    o.height = h
    o.max_frames = 1u32
    o.fixed_time = true
    o.headless = true
    o.theme = ThemeMode.Light
    o.scale = 1.0f
    o.font_file = asset_font()
    return o
}
func fixture(name : string_view) : path::PathBuf {
    var s = string("lang/tests/ui/fixtures/headless/")
    s.append_view(&name)
    return path::PathBuf.from(s.to_view())
}
func same_file(a : &path::Path, b : &path::Path) : bool {
    var ra = fs::read(a)
    var rb = fs::read(b)
    if(ra is Result.Err || rb is Result.Err) { return false }
    var Ok(da) = ra else return false
    var Ok(db) = rb else return false
    if(da.size() != db.size()) { return false }
    var i : size_t = 0u
    while(i < da.size()) {
        if(unsafe(da.get_ptr(i)[0]) != unsafe(db.get_ptr(i)[0])) { return false }
        i = i + 1u
    }
    return true
}

public struct HeadlessApp {
    var start_calls : u32
    var update_calls : u32
    var quit_after : u32
    var scene : u8
    var fail_view : bool
    var last_commands : u32
    var last_nodes : u32
    var last_paint_bytes : u64
}

impl Application for HeadlessApp {
    public func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit, UiError> {
        self.start_calls = self.start_calls + 1u32
        return Result.Ok<Unit, UiError>(Unit{})
    }
    public func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId, UiError> {
        if(self.fail_view) { return Result.Err<NodeId, UiError>(UiError.make(ErrorCode.Layout, "intentional view failure")) }
        var root = ui_internal::view_root(b)
        var canvas = b.row(key("canvas"), Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
        b.child(root, canvas)
        if(self.scene == 0u8) {
            var a = b.row(key("a"), Style.new().width(Dim.Px(120.0f)).height(Dim.Px(80.0f)).background(ColorToken.Primary).radius(Radius.MD))
            var c = b.row(key("c"), Style.new().width(Dim.Px(90.0f)).height(Dim.Px(60.0f)).background(ColorToken.Danger).radius(Radius.SM))
            var d = b.row(key("d"), Style.new().width(Dim.Px(160.0f)).height(Dim.Px(50.0f)).background(ColorToken.Success).radius(Radius.LG))
            b.child(canvas, a); b.child(canvas, c); b.child(canvas, d)
        } else {
            var wrap = b.wrap(key("grid"), Style.new().width(Dim.Px(960.0f)).height(Dim.Px(640.0f)).gap(Space.XS).wrap(WrapMode.Wrap))
            b.child(canvas, wrap)
            var i = 0u32
            while(i < 1000u32) {
                var token = ColorToken.Secondary
                if((i & 1u32) == 0u32) { token = ColorToken.Primary }
                var tile = b.row(key_of("tile", i as u64), Style.new().width(Dim.Px(16.0f)).height(Dim.Px(10.0f)).background(token))
                b.child(wrap, tile)
                i = i + 1u32
            }
        }
        return Result.Ok<NodeId, UiError>(root)
    }
    public func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit, UiError> {
        self.update_calls = self.update_calls + 1u32
        var st = ctx.stats()
        self.last_commands = st.commands
        self.last_nodes = st.nodes
        self.last_paint_bytes = st.paint_bytes
        if(self.quit_after != 0u32 && self.update_calls >= self.quit_after) { ctx.request_quit() }
        return Result.Ok<Unit, UiError>(Unit{})
    }
    public func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit, UiError> {
        return Result.Ok<Unit, UiError>(Unit{})
    }
    public func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction {
        return CloseAction.Allow
    }
}

func run_scene(name : string_view, scene : u8, w : i32, h : i32, update : bool) : bool {
    var expected = fixture(name)
    var out = path::PathBuf.from("/tmp/ui_headless_test.png")
    if(update) { out = expected }
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : scene, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var opts = options_for(w, h)
    var shot = run_headless_shot(&mut app, opts, out.as_view())
    if(shot is Result.Err) { return fail("shot-error") }
    var Ok(s) = shot else return false
    if(s.width != w as u32 || s.height != h as u32) { return fail("shot-size") }
    if(update) { return true }
    if(!same_file(&out.as_path(), &expected.as_path())) { return fail("shot-golden") }
    return true
}

func update_enabled() : bool {
    var e = environment::get("CHEMICAL_UI_UPDATE_GOLDENS")
    if(e is Option.Some) { var Some(v) = e else return false; return v.to_view().equals_with_len("1", 1u) }
    return false
}

func fixture_checks() : bool {
    var update = update_enabled()
    if(!run_scene("scene_3node.png", 0u8, 320, 200, update)) { return false }
    if(!run_scene("scene_1k.png", 1u8, 960, 640, update)) { return false }
    return true
}

func deterministic_checks() : bool {
    var a = path::PathBuf.from("/tmp/ui_headless_a.png")
    var b = path::PathBuf.from("/tmp/ui_headless_b.png")
    var app1 = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var app2 = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var o1 = options_for(320, 200)
    var o2 = options_for(320, 200)
    var s1 = run_headless_shot(&mut app1, o1, a.as_view())
    var s2 = run_headless_shot(&mut app2, o2, b.as_view())
    if(s1 is Result.Err || s2 is Result.Err) { return fail("deterministic-run") }
    if(!same_file(&a.as_path(), &b.as_path())) { return fail("deterministic-png") }
    var Ok(x) = s1 else return false
    var Ok(y) = s2 else return false
    if(x.hash != y.hash) { return fail("deterministic-hash") }
    return true
}

func pipeline_checks() : bool {
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var opts = options_for(160, 100)
    var r = run_headless(&mut app, opts)
    if(r is Result.Err) { return fail("pipeline-run") }
    if(app.start_calls != 1u32) { return fail("pipeline-start") }
    if(app.update_calls != 1u32) { return fail("pipeline-update") }
    if(app.last_commands == 0u32 || app.last_nodes < 4u32) { return fail("pipeline-stats") }
    if(app.last_paint_bytes != 160u64 * 100u64 * 4u64) { return fail("pipeline-bytes") }
    return true
}
func update_quit_checks() : bool {
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 2u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var opts = options_for(200, 120)
    opts.max_frames = 10u32
    var r = run_headless(&mut app, opts)
    if(r is Result.Err) { return fail("update-quit-run") }
    if(app.update_calls != 2u32) { return fail("update-count") }
    return true
}

func scale_checks() : bool {
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var opts = options_for(160, 100)
    opts.scale = 2.0f
    var out = path::PathBuf.from("/tmp/ui_headless_scale.png")
    var shot = run_headless_shot(&mut app, opts, out.as_view())
    if(shot is Result.Err) { return fail("scale-run") }
    var Ok(s) = shot else return false
    if(s.width != 320u32 || s.height != 200u32) { return fail("scale-size") }
    return true
}

func invalid_options() : bool {
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var opts = options_for(0, 100)
    var r = run_headless(&mut app, opts)
    if(r is Result.Ok) { return fail("invalid-width") }
    var Err(e) = r else return false
    if(e.code() != ErrorCode.InvalidArgument) { return fail("invalid-code") }
    var opts2 = options_for(100, 100)
    opts2.font_file = "lang/tests/ui/assets/does-not-exist.ttf"
    var r2 = run_headless(&mut app, opts2)
    if(r2 is Result.Ok) { return fail("missing-font") }
    return true
}

func error_path() : bool {
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : true, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var opts = options_for(100, 100)
    var r = run_headless(&mut app, opts)
    // P1.13: callback failure is contained by the error boundary.
    if(r is Result.Err) { return fail("error-view-contained") }
    return true
}

func is_release() : bool {
    var e = environment::get("CHEMICAL_UI_RELEASE")
    if(e is Option.Some) { var Some(v) = e else return false; return v.to_view().equals_with_len("1", 1u) }
    return false
}
func sorted5(a : *mut i64) {
    var i = 0
    while(i < 5) { var j = i + 1; while(j < 5) { if(unsafe(a[j]) < unsafe(a[i])) { var t = unsafe(a[i]); unsafe { a[i] = unsafe(a[j]); a[j] = t } }; j = j + 1 }; i = i + 1 }
}
func perf_checks() : bool {
    var app = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 0u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var t3 : [5]i64
    var i = 0
    while(i < 5) {
        var opts = options_for(960, 640)
        var out = path::PathBuf.from("/tmp/ui_headless_perf3.png")
        var s = std::chrono::Instant.now()
        var r = run_headless_shot(&mut app, opts, out.as_view())
        t3[i] = s.elapsed().as_micros()
        if(r is Result.Err) { return fail("perf-3node") }
        i = i + 1
    }
    sorted5(&raw mut t3[0])
    unsafe { printf("headless 3node startup_us p50=%ld p95=%ld\n", t3[2], t3[4]) }
    var app1k = HeadlessApp { start_calls : 0u32, update_calls : 0u32, quit_after : 0u32, scene : 1u8, fail_view : false, last_commands : 0u32, last_nodes : 0u32, last_paint_bytes : 0u64 }
    var t1 : [5]i64
    i = 0
    while(i < 5) {
        var opts = options_for(960, 640)
        var out = path::PathBuf.from("/tmp/ui_headless_perf1k.png")
        var s = std::chrono::Instant.now()
        var r = run_headless_shot(&mut app1k, opts, out.as_view())
        t1[i] = s.elapsed().as_micros()
        if(r is Result.Err) { return fail("perf-1k") }
        i = i + 1
    }
    sorted5(&raw mut t1[0])
    unsafe { printf("headless 1k startup_us p50=%ld p95=%ld\n", t1[2], t1[4]) }
    return true
}

func run_headless_tests() : bool {
    if(!fixture_checks()) { return false }
    if(!deterministic_checks()) { return false }
    if(!update_quit_checks()) { return false }
    if(!pipeline_checks()) { return false }
    if(!scale_checks()) { return false }
    if(!invalid_options()) { return false }
    if(!error_path()) { return false }
    if(is_release()) { if(!perf_checks()) { return false } }
    return true
}

} // namespace headless_tests
