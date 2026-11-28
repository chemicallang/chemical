// P1.14 Linux walking skeleton — GTK3/Cairo host for the P1.10 UI pipeline.
//
// GTK3 and cairo are used through hand-written extern declarations because this
// environment has the runtime libraries but no development headers/pkg-config.
// GTK owns the window; the draw callback renders the same ColorBuffer the
// headless pipeline produces and presents it through an ARGB32 cairo surface.
using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;

public namespace ui_native {

public type GtkWidget = window::GtkWidget
public type GdkWindow = window::GdkWindow
@no_init @extern public struct GtkContainer {}
@no_init @extern public struct cairo_t {}
@no_init @extern public struct cairo_surface_t {}

// ---------------------------------------------------------------------------
// GTK3 / cairo externs used by this host. Only the symbols the host calls are
// declared; no GTK headers are included.
// ---------------------------------------------------------------------------

@extern public unsafe func gtk_init_check(argc : *mut int, argv : *mut *mut *char) : int
@extern public unsafe func gtk_drawing_area_new() : *mut GtkWidget
@extern public unsafe func gtk_container_add(container : *mut GtkContainer, child : *mut GtkWidget)
@extern public unsafe func gtk_widget_set_hexpand(widget : *mut GtkWidget, expand : int)
@extern public unsafe func gtk_widget_set_vexpand(widget : *mut GtkWidget, expand : int)
@extern public unsafe func gtk_widget_queue_draw(widget : *mut GtkWidget)
@extern public unsafe func gtk_widget_get_allocated_width(widget : *mut GtkWidget) : int
@extern public unsafe func gtk_widget_get_allocated_height(widget : *mut GtkWidget) : int
@extern public unsafe func gtk_window_resize(window : *mut GtkWidget, width : int, height : int)
@extern public unsafe func gdk_test_simulate_button(window : *mut GdkWindow, x : int, y : int, button : u32, modifiers : u32, button_action : int) : int

@extern public unsafe func clock() : i64
@extern public unsafe func cairo_image_surface_create_for_data(data : *mut u8, format_ : int, width : int, height : int, stride : int) : *mut cairo_surface_t
@extern public unsafe func cairo_set_source_surface(cr : *mut cairo_t, surface : *mut cairo_surface_t, x : double, y : double)
@extern public unsafe func cairo_paint(cr : *mut cairo_t)
@extern public unsafe func cairo_surface_flush(surface : *mut cairo_surface_t)
@extern public unsafe func cairo_surface_destroy(surface : *mut cairo_surface_t)
@extern public unsafe func cairo_surface_write_to_png(surface : *mut cairo_surface_t, filename : *char) : int

// GTK3 has no gtk_widget_get_allocated_width on some very old builds; the
// runtime here is GTK 3.24 and exports both symbols.
comptime const CAIRO_FORMAT_ARGB32 : int = 0
comptime const GDK_BUTTON_PRESS : int = 4
comptime const GDK_BUTTON_RELEASE : int = 7
comptime const GDK_KEY_ESCAPE : int = 0xFF1B

public type GtkStartFn = (app : *mut void, frame : *FrameInfo, ctx : *mut UiCtx) => Result<Unit,UiError>
public type GtkViewFn = (app : *mut void, frame : *FrameInfo, b : *mut ViewBuilder) => Result<NodeId,UiError>
public type GtkUpdateFn = (app : *mut void, ev : *UiEvent, ctx : *mut UiCtx) => Result<Unit,UiError>
public type GtkCloseFn = (app : *mut void, frame : *FrameInfo, ctx : *mut UiCtx) => CloseAction

// ---------------------------------------------------------------------------
// Optional test driver. The host never schedules a periodic frame pump.
// ---------------------------------------------------------------------------
@direct_init
public struct GtkTestPlan {
    var click_after_ms : u32
    var click_x : int
    var click_y : int
    var direct_click : bool
    var resize_after_ms : u32
    var resize_w : int
    var resize_h : int
    var quit_after_ms : u32
    var idle_ms : u32
    var first_png : string_view
    var click_png : string_view
    var resize_png : string_view
}

@thread_local @never_destructed
var g_gtk_plan : GtkTestPlan

@thread_local @never_destructed
var g_gtk_host : *mut void = null

public func set_gtk_test_plan(plan : GtkTestPlan) {
    unsafe { g_gtk_plan = plan }
}

public func clear_gtk_test_plan() {
    unsafe {
        g_gtk_plan = GtkTestPlan {
            click_after_ms : 0u32, click_x : 0, click_y : 0, direct_click : false,
            resize_after_ms : 0u32, resize_w : 0, resize_h : 0,
            quit_after_ms : 0u32, idle_ms : 0u32,
            first_png : "", click_png : "", resize_png : ""
        }
    }
}

@retained
public func copy_view(v : string_view) : string {
    var out = string()
    out.append_view(&v)
    return out
}

func result_error_unit(r : Result<Unit,UiError>, fallback : UiError) : UiError {
    switch(r) { Err(e) => { return e } Ok(v) => { return fallback } }
}
func result_error_node(r : Result<NodeId,UiError>, fallback : UiError) : UiError {
    switch(r) { Err(e) => { return e } Ok(v) => { return fallback } }
}
func result_error_reconcile(r : Result<ui_internal::ReconcileStats,UiError>, fallback : UiError) : UiError {
    switch(r) { Err(e) => { return e } Ok(v) => { return fallback } }
}
func result_node(r : Result<NodeId,UiError>, fallback : NodeId) : NodeId {
    switch(r) { Err(e) => { return fallback } Ok(v) => { return v } }
}
func option_u64(o : Option<u64>, fallback : u64) : u64 {
    switch(o) { Some(v) => { return v } None() => { } }
    return fallback
}
func option_char(o : Option<char>, fallback : char) : char {
    switch(o) { Some(v) => { return v } None() => { } }
    return fallback
}
@retained
public func option_event(o : Option<UiEvent>, fallback : UiEvent) : UiEvent {
    switch(o) { Some(v) => { return v } None() => { } }
    return fallback
}
func key_from_gdk(keyval : int) : KeyCode {
    switch(keyval) {
        0xFF1B => return KeyCode.Escape
        0xFF09 => return KeyCode.Tab
        0xFF0D => return KeyCode.Enter
        0xFF08 => return KeyCode.Backspace
        0xFFFF => return KeyCode.Delete
        0xFF51 => return KeyCode.Left
        0xFF53 => return KeyCode.Right
        0xFF52 => return KeyCode.Up
        0xFF54 => return KeyCode.Down
        0x20 => return KeyCode.Space
        default => { }
    }
    if(keyval >= 97 && keyval <= 122) { return (KeyCode.A as int + (keyval - 97)) as KeyCode }
    if(keyval >= 65 && keyval <= 90) { return (KeyCode.A as int + (keyval - 65)) as KeyCode }
    if(keyval >= 48 && keyval <= 57) { return (KeyCode.Digit0 as int + (keyval - 48)) as KeyCode }
    return KeyCode.Unknown
}

func mods_from_event(ev : &window::Event) : Modifiers {
    return Modifiers {
        shift : (ev.modifiers & window::MOD_SHIFT) != 0,
        ctrl : (ev.modifiers & window::MOD_CTRL) != 0,
        alt : (ev.modifiers & window::MOD_ALT) != 0,
        meta : false, caps : false
    }
}

public func map_window_event(ev : &window::Event, scale : float) : Option<UiEvent> {
    var kind = EventKind.Frame
    switch(ev.kind) {
        window::EVENT_MOUSE_DOWN => { kind = EventKind.Press }
        window::EVENT_MOUSE_UP => { kind = EventKind.Release }
        window::EVENT_MOUSE_MOVE => { kind = EventKind.PointerMove }
        window::EVENT_MOUSE_WHEEL => { kind = EventKind.Scroll }
        window::EVENT_KEY_DOWN => { kind = EventKind.KeyDown }
        window::EVENT_KEY_UP => { kind = EventKind.KeyUp }
        default => { return Option.None<UiEvent>() }
    }
    var s = scale
    if(s <= 0.0f) { s = 1.0f }
    var out = UiEvent {
        window : zeroed<WindowId>(), key : zeroed<Key>(), kind : kind,
        payload : Payload.None(), modifiers : mods_from_event(ev),
        local_x : ev.x as float / s, local_y : ev.y as float / s,
        timestamp_ms : 0.0, frame : 0u64,
        pointer_button : ev.button as u8,
        scroll_dx : 0.0f, scroll_dy : 0.0f,
        key_code : KeyCode.Unknown, key_repeat : false, text_value : ""
    }
    if(kind == EventKind.Scroll) { out.scroll_dy = ev.y as float }
    if(kind == EventKind.KeyDown || kind == EventKind.KeyUp) { out.key_code = key_from_gdk(ev.key) }
    return Option.Some<UiEvent>(out)
}

func <A> app_start_wrapper(app : *mut void, frame : *FrameInfo, ctx : *mut UiCtx) : Result<Unit,UiError> where A : Application {
    unsafe {
        var ar : &mut A = unsafe(&mut *(app as *mut A))
        return ar.on_start(unsafe(&*frame), unsafe(&mut *ctx))
    }
}
func <A> app_view_wrapper(app : *mut void, frame : *FrameInfo, b : *mut ViewBuilder) : Result<NodeId,UiError> where A : Application {
    unsafe {
        var ar : &A = unsafe(&*(app as *mut A))
        var br : &mut ViewBuilder = unsafe(&mut *b)
        return ar.view(unsafe(&*frame), br)
    }
}
func <A> app_update_wrapper(app : *mut void, ev : *UiEvent, ctx : *mut UiCtx) : Result<Unit,UiError> where A : Application {
    unsafe {
        var ar : &mut A = unsafe(&mut *(app as *mut A))
        return ar.update(unsafe(&*ev), unsafe(&mut *ctx))
    }
}
func <A> app_close_wrapper(app : *mut void, frame : *FrameInfo, ctx : *mut UiCtx) : CloseAction where A : Application {
    unsafe {
        var ar : &mut A = unsafe(&mut *(app as *mut A))
        return ar.on_close(unsafe(&*frame), unsafe(&mut *ctx))
    }
}

// ---------------------------------------------------------------------------
// Host state. One local host per run; the GTK callback user data points at it
// for the lifetime of the main loop.
// ---------------------------------------------------------------------------
public struct GtkHost {
    var app : *mut void
    var start_fn : GtkStartFn
    var view_fn : GtkViewFn
    var update_fn : GtkUpdateFn
    var close_fn : GtkCloseFn
    var win : *mut window::Window
    var area : *mut GtkWidget
    var handle : *mut GtkWidget
    var options : Options
    var plan : GtkTestPlan
    var first_path : string
    var click_path : string
    var resize_path : string
    var frame : FrameInfo
    var viewport : Viewport
    var theme : Theme
    var builder : ViewBuilder
    var retained : ui_internal::RetainedTree
    var layout : ui_internal::LayoutState
    var layout_ctx : ui_internal::LayoutContext
    var display : ui_internal::DisplayState
    var buffer : ui_internal::ColorBuffer
    var bgra : vector<u8>
    var width : u32
    var height : u32
    var scale : float
    var focus : Option<Key>
    var started : bool
    var quit_requested : bool
    var last_error : Option<UiError>
    var pending : vector<UiEvent>
    var frames : u32
    var first_present_us : f64
    var first_present_clock : i64
    var first_pss_kb : u64
    var start_clock : i64
    var click_seen : bool
    var resize_seen : bool
    var saved_first : bool
    var saved_click : bool
    var saved_resize : bool
}

func host_options() : Options {
    return Options.make()
}

func host_reset(host : *mut GtkHost, w : u32, h : u32) {
    unsafe {
        var hp = host
        hp.width = w
        hp.height = h
        var lw = w as float
        var lh = h as float
        hp.viewport = Viewport { size : Size.new(lw, lh), safe_area : Insets.zero(), breakpoint : headless_breakpoint(lw) }
        hp.frame = headless_make_frame(&hp.options, hp.viewport, hp.scale, hp.frames as u64)
        var capacity = hp.options.limits.max_view_nodes
        if(capacity == 0u32) { capacity = 8192u32 }
        var display_cap = hp.options.limits.max_display_commands
        if(display_cap == 0u32) { display_cap = 8192u32 }
        hp.builder = ui_internal::new_view_builder(capacity, lw, lh)
        hp.retained = ui_internal::new_retained_tree(capacity)
        hp.layout = ui_internal::new_layout_state(capacity)
        hp.layout_ctx = ui_internal::new_layout_context(ui_internal::empty_text_context(), lw, lh)
        hp.display = ui_internal::DisplayState.new(display_cap)
        hp.buffer = ui_internal::ColorBuffer.new(w, h)
        hp.bgra.resize((w as size_t) * (h as size_t) * 4u)
    }
}

func host_error_view(host : *mut GtkHost) {
    unsafe {
        var hp = host
        ui_internal::reset_view_builder(&mut hp.builder)
        var root = ui_internal::view_root(&hp.builder)
        hp.builder.style(root, Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
        var card = hp.builder.column(key("__gtk_error"), Style.new().width(Dim.Px(300.0f)).height(Dim.Px(120.0f)).background(ColorToken.SurfaceRaised).border(Border { width : 2.0f, color : ColorToken.Danger }).radius(Radius.MD))
        hp.builder.child(root, card)
    }
}

@retained
func host_render(host : *mut GtkHost) : bool where A : Application {
    unsafe {
        var hp = host
        var aw = gtk_widget_get_allocated_width(hp.area)
        var ah = gtk_widget_get_allocated_height(hp.area)
        if(aw <= 0 || ah <= 0) { return false }
        var w = aw as u32
        var h = ah as u32
        if(w != hp.width || h != hp.height) { host_reset(hp, w, h) }
        hp.frame.frame = hp.frames as u64
        hp.frame.viewport = hp.viewport
        if(!hp.options.fixed_time) { hp.frame.now_ms = hp.frames as double * 16.0 } else { hp.frame.now_ms = 0.0 }
        var ctx = ui_ctx_new(hp.frame, hp.viewport, hp.theme, ui_internal::empty_text_context(), headless_zero_stats(), hp.focus, hp.frame.text_scale)
        if(!hp.started) {
            var sr = hp.start_fn(hp.app, &hp.frame as *FrameInfo, &mut ctx as *mut UiCtx)
            if(sr is Result.Err) {
                hp.last_error = Option.Some<UiError>(result_error_unit(sr, UiError.make(ErrorCode.Internal, "on_start corrupt")))
            } else {
                hp.theme = ui_ctx_theme(&ctx)
                hp.focus = ui_ctx_focus(&ctx)
            }
            hp.started = true
        }
        var i : size_t = 0u
        while(i < hp.pending.size()) {
            var ev = unsafe(*hp.pending.get_ptr(i))
            var ur = hp.update_fn(hp.app, &ev as *UiEvent, &mut ctx as *mut UiCtx)
            if(ur is Result.Err) {
                hp.last_error = Option.Some<UiError>(result_error_unit(ur, UiError.make(ErrorCode.Internal, "update corrupt")))
            }
            i = i + 1u
        }
        hp.pending.clear()
        var reported = ui_ctx_reported(&mut ctx)
        if(reported.is_some()) { hp.last_error = reported }
        if(ui_ctx_quit(&ctx)) { hp.quit_requested = true }
        hp.theme = ui_ctx_theme(&ctx)
        hp.focus = ui_ctx_focus(&ctx)
        hp.builder.frame_data = hp.frame
        hp.builder.viewport_data = hp.viewport
        ui_internal::reset_view_builder(&mut hp.builder)
        if(hp.last_error.is_some()) {
            host_error_view(hp)
        } else {
            var vr = hp.view_fn(hp.app, &hp.frame as *FrameInfo, &mut hp.builder as *mut ViewBuilder)
            if(vr is Result.Err) {
                hp.last_error = Option.Some<UiError>(result_error_node(vr, UiError.make(ErrorCode.Internal, "view corrupt")))
                host_error_view(hp)
            } else {
                var root = result_node(vr, zeroed<NodeId>())
                if(!root.is_valid() || ui_internal::view_diagnostic_count(&hp.builder) != 0u32) {
                    hp.last_error = Option.Some<UiError>(UiError.make(ErrorCode.Layout, "view rejected"))
                    host_error_view(hp)
                } else {
                    var be = ui_internal::view_first_error(&hp.builder)
                    if(be.is_some()) {
                        hp.last_error = be
                        host_error_view(hp)
                    }
                }
            }
        }
        var rec = ui_internal::reconcile(&mut hp.retained, &hp.builder)
        if(rec is Result.Err) {
            hp.last_error = Option.Some<UiError>(result_error_reconcile(rec, UiError.make(ErrorCode.Internal, "reconcile corrupt")))
            host_error_view(hp)
            var rec2 = ui_internal::reconcile(&mut hp.retained, &hp.builder)
            if(rec2 is Result.Err) { return false }
        }
        var _ls = ui_internal::layout_run(&mut hp.layout, &hp.builder, &hp.layout_ctx, &hp.theme)
        if(ui_internal::layout_warning_count(&hp.layout) != 0u32) {
            hp.last_error = Option.Some<UiError>(UiError.make(ErrorCode.Layout, "layout warning"))
            host_error_view(hp)
            _ls = ui_internal::layout_run(&mut hp.layout, &hp.builder, &hp.layout_ctx, &hp.theme)
        }
        hp.display.focus_key = hp.focus
        hp.display.hover_key = Option.None<Key>()
        var _commands = ui_internal::display_build(&mut hp.display, &hp.builder, &hp.layout, &hp.theme, &hp.layout_ctx.text, Size.new(hp.width as float, hp.height as float))
        var raster = ui_internal::Rasterizer.new(&raw mut hp.buffer)
        var _rs = raster.render(&hp.display.list)
        hp.frames = hp.frames + 1u32
        return true
    }
}

@retained
func host_present(host : *mut GtkHost, cr : *mut cairo_t) : int {
    unsafe {
        var hp = host
        var src = ui_internal::raster_buffer_bytes(&hp.buffer)
        var n : size_t = (hp.width as size_t) * (hp.height as size_t) * 4u
        var i : size_t = 0u
        while(i + 3u < n && i + 3u < src.size()) {
            var r = unsafe(src.data()[i])
            var g = unsafe(src.data()[i + 1u])
            var b = unsafe(src.data()[i + 2u])
            var a = unsafe(src.data()[i + 3u])
            hp.bgra.mutable_data()[i] = b
            hp.bgra.mutable_data()[i + 1u] = g
            hp.bgra.mutable_data()[i + 2u] = r
            hp.bgra.mutable_data()[i + 3u] = a
            i = i + 4u
        }
        var surface = cairo_image_surface_create_for_data(hp.bgra.mutable_data() as *mut u8, CAIRO_FORMAT_ARGB32, hp.width as int, hp.height as int, (hp.width as int) * 4)
        if(surface == null) { return 1 }
        cairo_set_source_surface(cr, surface, 0.0, 0.0)
        cairo_paint(cr)
        cairo_surface_flush(surface)
        if(!hp.saved_first && hp.plan.first_png.size() > 0u) {
            cairo_surface_write_to_png(surface, hp.first_path.data())
            hp.saved_first = true
        }
        if(hp.click_seen && !hp.saved_click && hp.plan.click_png.size() > 0u) {
            cairo_surface_write_to_png(surface, hp.click_path.data())
            hp.saved_click = true
        }
        if(hp.resize_seen && !hp.saved_resize && hp.plan.resize_png.size() > 0u) {
            cairo_surface_write_to_png(surface, hp.resize_path.data())
            hp.saved_resize = true
        }
        cairo_surface_destroy(surface)
        return 1
    }
}

@retained
public func gtk_draw_cb(widget : *mut GtkWidget, cr : *mut cairo_t, data : *mut void) : int where A : Application {
    unsafe {
        var host = data as *mut GtkHost
        if(host == null || cr == null) { return 1 }
        var hp = host
        var _ = host_render(host)
        var _p = host_present(host, cr)
        if(hp.first_present_us == 0.0) {
            hp.first_present_us = (clock() - hp.start_clock) as double / 1000.0
            hp.first_present_clock = clock()
            hp.first_pss_kb = pss_kb()
        }
        if(hp.quit_requested) { window::raw::window_quit() }
        if(hp.options.max_frames > 0u32 && hp.frames >= hp.options.max_frames) { window::raw::window_quit() }
        return 1
    }
}

@retained
public func gtk_host_event(host : *mut GtkHost, ev : &window::Event) {
    unsafe {
        var hp = host
        if(ev.kind == window::EVENT_KEY_DOWN && ev.key == GDK_KEY_ESCAPE) {
            hp.quit_requested = true
            window::raw::window_quit()
            return
        }
        var mapped = map_window_event(ev, hp.scale)
        if(mapped.is_some()) {
            hp.pending.push(option_event(mapped, UiEvent { window : zeroed<WindowId>(), key : zeroed<Key>(), kind : EventKind.Frame, payload : Payload.None(), modifiers : Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false }, local_x : 0.0f, local_y : 0.0f, timestamp_ms : 0.0, frame : 0u64, pointer_button : 0u8, scroll_dx : 0.0f, scroll_dy : 0.0f, key_code : KeyCode.Unknown, key_repeat : false, text_value : "" }))
            gtk_widget_queue_draw(hp.area)
        }
    }
}

@retained
public func gtk_host_resize(host : *mut GtkHost, w : int, h : int) {
    unsafe {
        var hp = host
        if(w > 0 && h > 0) { gtk_widget_queue_draw(hp.area) }
    }
}

@retained
func gtk_host_inject_click(host : *mut GtkHost, x : int, y : int) {
    unsafe {
        var hp = host
        var down = window::Event.make(); down.kind = window::EVENT_MOUSE_DOWN; down.x = x; down.y = y; down.button = window::MOUSE_LEFT
        gtk_host_event(hp, &down)
        var up = window::Event.make(); up.kind = window::EVENT_MOUSE_UP; up.x = x; up.y = y; up.button = window::MOUSE_LEFT
        gtk_host_event(hp, &up)
    }
}

@retained
public func gtk_area_button(widget : *mut GtkWidget, event : *mut window::GdkEventButton, data : *mut void, kind : int) : int {
    unsafe {
        var host = data as *mut GtkHost
        if(host == null || event == null) { return 0 }
        var ev = window::Event.make()
        ev.kind = kind
        ev.x = event.x as int
        ev.y = event.y as int
        ev.button = event.button as int
        ev.modifiers = 0
        if((event.state & 0x1u32) != 0u32) { ev.modifiers |= window::MOD_SHIFT }
        if((event.state & 0x4u32) != 0u32) { ev.modifiers |= window::MOD_CTRL }
        if((event.state & 0x8u32) != 0u32) { ev.modifiers |= window::MOD_ALT }
        gtk_host_event(host, &ev)
        return 0
    }
}
@retained
public func gtk_area_press(widget : *mut GtkWidget, event : *mut window::GdkEventButton, data : *mut void) : int { return gtk_area_button(widget, event, data, window::EVENT_MOUSE_DOWN) }
@retained
public func gtk_area_release(widget : *mut GtkWidget, event : *mut window::GdkEventButton, data : *mut void) : int { return gtk_area_button(widget, event, data, window::EVENT_MOUSE_UP) }
@retained
public func gtk_area_motion(widget : *mut GtkWidget, event : *mut window::GdkEventMotion, data : *mut void) : int {
    unsafe {
        var host = data as *mut GtkHost
        if(host == null || event == null) { return 0 }
        var ev = window::Event.make(); ev.kind = window::EVENT_MOUSE_MOVE; ev.x = event.x as int; ev.y = event.y as int; ev.button = window::MOUSE_LEFT
        gtk_host_event(host, &ev); return 0
    }
}
@retained
public func gtk_area_scroll(widget : *mut GtkWidget, event : *mut window::GdkEventScroll, data : *mut void) : int {
    unsafe {
        var host = data as *mut GtkHost
        if(host == null || event == null) { return 0 }
        var ev = window::Event.make(); ev.kind = window::EVENT_MOUSE_WHEEL; ev.x = event.x as int; ev.y = (event.delta_y * 100.0) as int
        gtk_host_event(host, &ev); return 0
    }
}

@retained
public func gtk_click_timer(data : *mut void) : int {
    unsafe {
        if(g_gtk_host == null) { return 0 }
        var host = data as *mut GtkHost
        if(host == null) { return 0 }
        var hp = host
        hp.click_seen = true
        if(hp.plan.direct_click) {
            gtk_host_inject_click(hp, hp.plan.click_x, hp.plan.click_y)
            gtk_widget_queue_draw(hp.area)
            return 0
        }
        var gdkwin = window::gtk_widget_get_window(hp.area)
        if(gdkwin != null) {
            var r1 = gdk_test_simulate_button(gdkwin, hp.plan.click_x, hp.plan.click_y, 1u32, 0u32, GDK_BUTTON_PRESS)
            var r2 = gdk_test_simulate_button(gdkwin, hp.plan.click_x, hp.plan.click_y, 1u32, 0u32, GDK_BUTTON_RELEASE)
            var top = window::gtk_widget_get_window(hp.handle)
            if(top != null) {
                var t1 = gdk_test_simulate_button(top, hp.plan.click_x, hp.plan.click_y, 1u32, 0u32, GDK_BUTTON_PRESS)
                var t2 = gdk_test_simulate_button(top, hp.plan.click_x, hp.plan.click_y, 1u32, 0u32, GDK_BUTTON_RELEASE)
            }
            if(r1 == 0) { gtk_host_inject_click(hp, hp.plan.click_x, hp.plan.click_y) }
        } else { gtk_host_inject_click(hp, hp.plan.click_x, hp.plan.click_y) }
        gtk_widget_queue_draw(hp.area)
        return 0
    }
}

@retained
public func gtk_resize_timer(data : *mut void) : int {
    unsafe {
        if(g_gtk_host == null) { return 0 }
        var host = data as *mut GtkHost
        if(host == null) { return 0 }
        var hp = host
        hp.resize_seen = true
        gtk_window_resize(hp.handle, hp.plan.resize_w, hp.plan.resize_h)
        return 0
    }
}

@retained
public func gtk_quit_timer(data : *mut void) : int {
    unsafe { window::raw::window_quit() }
    return 0
}

@retained
func pss_parse(v : string_view) : u64 {
    var pos = v.find("VmRSS:")
    if(pos.is_none()) { return 0u64 }
    var at = option_u64(pos, 0u64)
    if(at == 0u64) { return 0u64 }
    var i = at + 6u
    var n : u64 = 0u64
    var any = false
    while(i < v.size()) {
        var oc = v.get(i)
        if(oc.is_none()) { break }
        var ch = option_char(oc, 0 as char)
        if(ch == 0 as char) { break }
        if(ch >= '0' && ch <= '9') { n = n * 10u64 + (ch as u64 - 48u64); any = true }
        else if(any) { break }
        i = i + 1u
    }
    return n
}
@retained
public func pss_kb() : u64 {
    var pp = path::PathBuf.from("/proc/self/status")
    var rd = fs::read_to_string(&pp.as_path())
    switch(rd) {
        Ok(t) => { return pss_parse(t.to_view()) }
        Err(e) => { return 0u64 }
    }
}

@retained
public func <A> run_gtk(app : &mut A, options : Options) : Result<Unit, UiError> where A : Application {
    var run_start_clock = unsafe(clock())
    var argc : int = 0
    var ar = unsafe(gtk_init_check(&raw mut argc, null))
    if(ar == 0) { return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Platform, "GTK3 display unavailable")) }
    var created = window::Window.new("Chemical UI P1.14", options.width, options.height)
    if(created is Result.Err) { return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Platform, "GTK3 window creation failed")) }
    var Ok(win) = created else return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Platform, "GTK3 window result corrupt"))
    var scale = options.scale
    if(scale <= 0.0f) { scale = 1.0f }
    var logical_w = options.width as float
    var logical_h = options.height as float
    var physical_w = (logical_w * scale) as u32
    var physical_h = (logical_h * scale) as u32
    if(physical_w == 0u32 || physical_h == 0u32) { win.close(); return Result.Err<Unit,UiError>(UiError.make(ErrorCode.InvalidArgument, "GTK surface is empty")) }
    var capacity = options.limits.max_view_nodes
    if(capacity == 0u32) { capacity = 8192u32 }
    var display_cap = options.limits.max_display_commands
    if(display_cap == 0u32) { display_cap = 8192u32 }
    var plan = unsafe { g_gtk_plan }
    var host = GtkHost {
        app : app as *mut void, start_fn : app_start_wrapper<A>, view_fn : app_view_wrapper<A>,
        update_fn : app_update_wrapper<A>, close_fn : app_close_wrapper<A>,
        win : &raw mut win, area : null, handle : null,
        options : options, plan : plan,
        first_path : copy_view(plan.first_png), click_path : copy_view(plan.click_png), resize_path : copy_view(plan.resize_png),
        frame : headless_make_frame(&options, Viewport { size : Size.new(logical_w, logical_h), safe_area : Insets.zero(), breakpoint : headless_breakpoint(logical_w) }, scale, 0u64),
        viewport : Viewport { size : Size.new(logical_w, logical_h), safe_area : Insets.zero(), breakpoint : headless_breakpoint(logical_w) },
        theme : headless_theme(&options),
        builder : ui_internal::new_view_builder(capacity, logical_w, logical_h),
        retained : ui_internal::new_retained_tree(capacity),
        layout : ui_internal::new_layout_state(capacity),
        layout_ctx : ui_internal::new_layout_context(ui_internal::empty_text_context(), logical_w, logical_h),
        display : ui_internal::DisplayState.new(display_cap),
        buffer : ui_internal::ColorBuffer.new(physical_w, physical_h),
        bgra : vector<u8>(),
        width : physical_w, height : physical_h, scale : scale,
        focus : Option.None<Key>(), started : false, quit_requested : false,
        last_error : Option.None<UiError>(), pending : vector<UiEvent>(),
        frames : 0u32, first_present_us : 0.0, first_present_clock : 0, first_pss_kb : 0u64, start_clock : run_start_clock,
        click_seen : false, resize_seen : false,
        saved_first : false, saved_click : false, saved_resize : false
    }
    unsafe { g_gtk_host = &raw mut host as *mut void }
    win.on_event(||(ev : &window::Event) => {
        unsafe {
            var hp = g_gtk_host as *mut GtkHost
            if(hp != null) { gtk_host_event(hp, ev) }
        }
    })
    win.on_resize(||(width : int, height : int) => {
        unsafe {
            var hp = g_gtk_host as *mut GtkHost
            if(hp != null) { gtk_host_resize(hp, width, height) }
        }
    })
    win.on_close(||() => {
        unsafe { window::raw::window_quit() }
    })
    var handle = unsafe(win.native_handle())
    if(handle == null) { win.close(); return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Platform, "GTK window handle unavailable")) }
    host.handle = handle as *mut GtkWidget
    var area = unsafe(gtk_drawing_area_new())
    if(area == null) { win.close(); return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Platform, "GTK drawing area creation failed")) }
    host.area = area
    unsafe {
        gtk_container_add(handle as *mut GtkContainer, area)
        gtk_widget_set_hexpand(area, 1)
        gtk_widget_set_vexpand(area, 1)
        var _ = window::g_signal_connect_data(area as *mut void, "draw\0" as *char, gtk_draw_cb as *mut void, &raw mut host as *mut void, null, 0)
        window::gtk_widget_add_events(area, window::GDK_BUTTON_PRESS_MASK | window::GDK_BUTTON_RELEASE_MASK | window::GDK_POINTER_MOTION_MASK | window::GDK_SCROLL_MASK)
        var _abp = window::g_signal_connect_data(area as *mut void, "button-press-event\0" as *char, gtk_area_press as *mut void, &raw mut host as *mut void, null, 0)
        var _abr = window::g_signal_connect_data(area as *mut void, "button-release-event\0" as *char, gtk_area_release as *mut void, &raw mut host as *mut void, null, 0)
        var _amm = window::g_signal_connect_data(area as *mut void, "motion-notify-event\0" as *char, gtk_area_motion as *mut void, &raw mut host as *mut void, null, 0)
        var _asc = window::g_signal_connect_data(area as *mut void, "scroll-event\0" as *char, gtk_area_scroll as *mut void, &raw mut host as *mut void, null, 0)
    }
    win.show()
    if(plan.click_after_ms > 0u32) { unsafe { var _c = window::raw::g_timeout_add(plan.click_after_ms, gtk_click_timer as *mut void, &raw mut host as *mut void) } }
    if(plan.resize_after_ms > 0u32) { unsafe { var _r = window::raw::g_timeout_add(plan.resize_after_ms, gtk_resize_timer as *mut void, &raw mut host as *mut void) } }
    if(plan.quit_after_ms > 0u32) { unsafe { var _q = window::raw::g_timeout_add(plan.quit_after_ms, gtk_quit_timer as *mut void, &raw mut host as *mut void) } }
    unsafe { window::raw::window_run() }
    unsafe { g_gtk_host = null }
    var end_clock = unsafe(clock())
    var ctx = ui_ctx_new(host.frame, host.viewport, host.theme, ui_internal::empty_text_context(), headless_zero_stats(), host.focus, host.frame.text_scale)
    var _close = host.close_fn(host.app, &host.frame as *FrameInfo, &mut ctx as *mut UiCtx)
    if(options.debug) {
        unsafe {
            printf("ui_native gtk startup_us=%.0f pss_kb=%llu frames=%u idle_cpu_ms=%.3f\n",
                   host.first_present_us, host.first_pss_kb, host.frames,
                   ((end_clock - host.first_present_clock) as double) / 1000.0)
        }
    }
    if(unsafe(window::raw::window_quit_by_destroy()) == 0) { win.close() }
    return Result.Ok<Unit,UiError>(Unit{})
}

} // namespace ui_native
