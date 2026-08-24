// window_test.ch — Tests for the window library.
// These tests exercise the window API surface without requiring a display
// server. They verify struct defaults, getter/setter pairs, and utility
// functions. Window creation/display is tested separately in the webview
// test suite (which requires GTK3 or a Win32 environment).

using std::string;
using std::Result;

// ---------------------------------------------------------------------------
// Window struct defaults
// ---------------------------------------------------------------------------

@test
public func test_window_make_defaults(env : &mut TestEnv) {
    var w = window::Window.make()
    if(w.width != 800) { env.error("default width should be 800"); return }
    if(w.height != 600) { env.error("default height should be 600"); return }
    if(w.x != 0) { env.error("default x should be 0"); return }
    if(w.y != 0) { env.error("default y should be 0"); return }
    if(w.min_w != 0) { env.error("default min_w should be 0"); return }
    if(w.min_h != 0) { env.error("default min_h should be 0"); return }
    if(w.max_w != 0) { env.error("default max_w should be 0"); return }
    if(w.max_h != 0) { env.error("default max_h should be 0"); return }
    if(w.fullscreen) { env.error("default fullscreen should be false"); return }
    if(w.visible) { env.error("default visible should be false"); return }
    if(!w.decorated) { env.error("default decorated should be true"); return }
    if(w.always_on_top) { env.error("default always_on_top should be false"); return }
    if(w.created) { env.error("default created should be false"); return }
    if(w.cursor != window::CURSOR_ARROW) { env.error("default cursor should be CURSOR_ARROW"); return }
    if(w.dpi != 96) { env.error("default dpi should be 96"); return }
}

// ---------------------------------------------------------------------------
// Title
// ---------------------------------------------------------------------------

@test
public func test_window_title_default(env : &mut TestEnv) {
    var w = window::Window.make()
    var t = window::window_title(&raw mut w)
    if(t.size() != 0) { env.error("default title should be empty"); return }
}

@test
public func test_window_set_title(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_title(&raw mut w, "Hello World\0" as *char)
    var t = window::window_title(&raw mut w)
    if(t.size() != 11) { env.error("title length should be 11"); return }
    if(t.get(0) != 'H' as char) { env.error("title should start with H"); return }
}

// ---------------------------------------------------------------------------
// Size (logical)
// ---------------------------------------------------------------------------

@test
public func test_window_size_default(env : &mut TestEnv) {
    var w = window::Window.make()
    var sz = window::window_size(&raw mut w)
    if(sz.width != 800) { env.error("default size width should be 800"); return }
    if(sz.height != 600) { env.error("default size height should be 600"); return }
}

@test
public func test_window_set_size(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_size(&raw mut w, 1024, 768)
    // Before creation, set_size stores the values.
    if(w.width != 1024) { env.error("width should be 1024 after set_size"); return }
    if(w.height != 768) { env.error("height should be 768 after set_size"); return }
    var sz = window::window_size(&raw mut w)
    if(sz.width != 1024) { env.error("window_size width should be 1024"); return }
    if(sz.height != 768) { env.error("window_size height should be 768"); return }
}

// ---------------------------------------------------------------------------
// Client size (before creation = logical size)
// ---------------------------------------------------------------------------

@test
public func test_window_client_size_before_create(env : &mut TestEnv) {
    var w = window::Window.make()
    var csz = window::window_client_size(&raw mut w)
    if(csz.width != 800) { env.error("client_size width before create should be 800"); return }
    if(csz.height != 600) { env.error("client_size height before create should be 600"); return }
}

// ---------------------------------------------------------------------------
// Position
// ---------------------------------------------------------------------------

@test
public func test_window_position_default(env : &mut TestEnv) {
    var w = window::Window.make()
    var pos = window::window_position(&raw mut w)
    if(pos.x != 0) { env.error("default position x should be 0"); return }
    if(pos.y != 0) { env.error("default position y should be 0"); return }
}

@test
public func test_window_set_position(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_position(&raw mut w, 100, 200)
    if(w.x != 100) { env.error("x should be 100 after set_position"); return }
    if(w.y != 200) { env.error("y should be 200 after set_position"); return }
    var pos = window::window_position(&raw mut w)
    if(pos.x != 100) { env.error("window_position x should be 100"); return }
    if(pos.y != 200) { env.error("window_position y should be 200"); return }
}

// ---------------------------------------------------------------------------
// Min / Max size
// ---------------------------------------------------------------------------

@test
public func test_window_min_max_size_default(env : &mut TestEnv) {
    var w = window::Window.make()
    var minsz = window::window_min_size(&raw mut w)
    if(minsz.width != 0) { env.error("default min_w should be 0"); return }
    if(minsz.height != 0) { env.error("default min_h should be 0"); return }
    var maxsz = window::window_max_size(&raw mut w)
    if(maxsz.width != 0) { env.error("default max_w should be 0"); return }
    if(maxsz.height != 0) { env.error("default max_h should be 0"); return }
}

@test
public func test_window_set_min_max_size(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_min_size(&raw mut w, 200, 150)
    window::window_set_max_size(&raw mut w, 1920, 1080)
    var minsz = window::window_min_size(&raw mut w)
    if(minsz.width != 200) { env.error("min_w should be 200"); return }
    if(minsz.height != 150) { env.error("min_h should be 150"); return }
    var maxsz = window::window_max_size(&raw mut w)
    if(maxsz.width != 1920) { env.error("max_w should be 1920"); return }
    if(maxsz.height != 1080) { env.error("max_h should be 1080"); return }
}

// ---------------------------------------------------------------------------
// Visual state (before creation — stored in struct)
// ---------------------------------------------------------------------------

@test
public func test_window_fullscreen(env : &mut TestEnv) {
    var w = window::Window.make()
    if(window::window_is_fullscreen(&raw mut w)) { env.error("should not be fullscreen by default"); return }
    // Before creation, set_fullscreen returns early (no widget), so we only
    // verify the default state.
}

@test
public func test_window_opacity(env : &mut TestEnv) {
    var w = window::Window.make()
    if(w.opacity != 1.0) { env.error("default opacity should be 1.0"); return }
    window::window_set_opacity(&raw mut w, 0.5)
    if(w.opacity != 0.5) { env.error("opacity should be 0.5 after set"); return }
}

@test
public func test_window_always_on_top(env : &mut TestEnv) {
    var w = window::Window.make()
    if(w.always_on_top) { env.error("default always_on_top should be false"); return }
    window::window_set_always_on_top(&raw mut w, true)
    if(!w.always_on_top) { env.error("always_on_top should be true after set"); return }
    window::window_set_always_on_top(&raw mut w, false)
    if(w.always_on_top) { env.error("always_on_top should be false after unset"); return }
}

@test
public func test_window_decorated(env : &mut TestEnv) {
    var w = window::Window.make()
    if(!w.decorated) { env.error("default decorated should be true"); return }
    window::window_set_decorated(&raw mut w, false)
    if(w.decorated) { env.error("decorated should be false after set"); return }
    window::window_set_decorated(&raw mut w, true)
    if(!w.decorated) { env.error("decorated should be true after unset"); return }
}

@test
public func test_window_cursor(env : &mut TestEnv) {
    var w = window::Window.make()
    if(w.cursor != window::CURSOR_ARROW) { env.error("default cursor should be CURSOR_ARROW"); return }
    window::window_set_cursor(&raw mut w, window::CURSOR_IBEAM)
    if(w.cursor != window::CURSOR_IBEAM) { env.error("cursor should be CURSOR_IBEAM after set"); return }
}

// ---------------------------------------------------------------------------
// DPI / scale
// ---------------------------------------------------------------------------

@test
public func test_window_dpi_default(env : &mut TestEnv) {
    var w = window::Window.make()
    var dpi = window::window_dpi(&raw mut w)
    if(dpi != 96) { env.error("default dpi should be 96"); return }
}

@test
public func test_window_scale_factor(env : &mut TestEnv) {
    var w = window::Window.make()
    var sf = window::window_scale_factor(&raw mut w)
    // scale_factor = dpi / 96.0; default dpi = 96, so scale = 1.0
    if(sf != 1.0) { env.error("default scale_factor should be 1.0"); return }
}

// ---------------------------------------------------------------------------
// Monitor info (these call into GDK/Win32 — should work on any machine
// with a display server)
// ---------------------------------------------------------------------------

@test
public func test_window_monitor_count(env : &mut TestEnv) {
    // On headless/CI machines there may be no display server.
    // We just verify the function doesn't crash.
    var count = window::window_monitor_count()
    if(count < 0) { env.error("monitor_count should not be negative"); return }
}

@test
public func test_window_monitor_bounds(env : &mut TestEnv) {
    // On headless/CI machines there may be no display server.
    // We just verify the function doesn't crash.
    var bounds = window::window_monitor_bounds(0)
    if(bounds.width < 0) { env.error("monitor bounds width should not be negative"); return }
    if(bounds.height < 0) { env.error("monitor bounds height should not be negative"); return }
}

@test
public func test_window_monitor_scale(env : &mut TestEnv) {
    var scale = window::window_monitor_scale(0)
    if(scale < 1.0) { env.error("monitor scale should be >= 1.0"); return }
}

// ---------------------------------------------------------------------------
// Created state
// ---------------------------------------------------------------------------

@test
public func test_window_is_created_before_create(env : &mut TestEnv) {
    var w = window::Window.make()
    if(window::window_is_created(&raw mut w)) { env.error("should not be created before window_create"); return }
}

@test
public func test_window_native_handle_before_create(env : &mut TestEnv) {
    var w = window::Window.make()
    var h = window::window_native_handle(&raw mut w)
    if(h != null) { env.error("native handle should be null before creation"); return }
}

// ---------------------------------------------------------------------------
// Geometry types
// ---------------------------------------------------------------------------

@test
public func test_size_make(env : &mut TestEnv) {
    var sz = window::Size.make(100, 200)
    if(sz.width != 100) { env.error("Size.width should be 100"); return }
    if(sz.height != 200) { env.error("Size.height should be 200"); return }
}

@test
public func test_position_make(env : &mut TestEnv) {
    var pos = window::Position.make(10, 20)
    if(pos.x != 10) { env.error("Position.x should be 10"); return }
    if(pos.y != 20) { env.error("Position.y should be 20"); return }
}

@test
public func test_rect_make(env : &mut TestEnv) {
    var r = window::Rect.make(5, 10, 300, 400)
    if(r.x != 5) { env.error("Rect.x should be 5"); return }
    if(r.y != 10) { env.error("Rect.y should be 10"); return }
    if(r.width != 300) { env.error("Rect.width should be 300"); return }
    if(r.height != 400) { env.error("Rect.height should be 400"); return }
}

@test
public func test_event_make(env : &mut TestEnv) {
    var ev = window::Event.make()
    if(ev.kind != 0) { env.error("Event.kind default should be 0"); return }
    if(ev.x != 0) { env.error("Event.x default should be 0"); return }
    if(ev.y != 0) { env.error("Event.y default should be 0"); return }
    if(ev.button != 0) { env.error("Event.button default should be 0"); return }
    if(ev.key != 0) { env.error("Event.key default should be 0"); return }
    if(ev.key_char != 0) { env.error("Event.key_char default should be 0"); return }
    if(ev.modifiers != 0) { env.error("Event.modifiers default should be 0"); return }
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

@test
public func test_window_event_constants(env : &mut TestEnv) {
    if(window::EVENT_KEY_DOWN != 0) { env.error("EVENT_KEY_DOWN should be 0"); return }
    if(window::EVENT_KEY_UP != 1) { env.error("EVENT_KEY_UP should be 1"); return }
    if(window::EVENT_CHAR != 2) { env.error("EVENT_CHAR should be 2"); return }
    if(window::EVENT_MOUSE_DOWN != 3) { env.error("EVENT_MOUSE_DOWN should be 3"); return }
    if(window::EVENT_MOUSE_UP != 4) { env.error("EVENT_MOUSE_UP should be 4"); return }
    if(window::EVENT_MOUSE_MOVE != 5) { env.error("EVENT_MOUSE_MOVE should be 5"); return }
    if(window::EVENT_MOUSE_WHEEL != 6) { env.error("EVENT_MOUSE_WHEEL should be 6"); return }
    if(window::MOUSE_LEFT != 0) { env.error("MOUSE_LEFT should be 0"); return }
    if(window::MOUSE_RIGHT != 1) { env.error("MOUSE_RIGHT should be 1"); return }
    if(window::MOUSE_MIDDLE != 2) { env.error("MOUSE_MIDDLE should be 2"); return }
    if(window::MOD_SHIFT != 1) { env.error("MOD_SHIFT should be 1"); return }
    if(window::MOD_CTRL != 2) { env.error("MOD_CTRL should be 2"); return }
    if(window::MOD_ALT != 4) { env.error("MOD_ALT should be 4"); return }
}

@test
public func test_window_cursor_constants(env : &mut TestEnv) {
    if(window::CURSOR_ARROW != 0) { env.error("CURSOR_ARROW should be 0"); return }
    if(window::CURSOR_IBEAM != 1) { env.error("CURSOR_IBEAM should be 1"); return }
    if(window::CURSOR_WAIT != 2) { env.error("CURSOR_WAIT should be 2"); return }
    if(window::CURSOR_CROSS != 3) { env.error("CURSOR_CROSS should be 3"); return }
    if(window::CURSOR_HAND != 4) { env.error("CURSOR_HAND should be 4"); return }
    if(window::CURSOR_HELP != 5) { env.error("CURSOR_HELP should be 5"); return }
    if(window::CURSOR_MOVE != 6) { env.error("CURSOR_MOVE should be 6"); return }
    if(window::CURSOR_RESIZE_NS != 7) { env.error("CURSOR_RESIZE_NS should be 7"); return }
    if(window::CURSOR_RESIZE_EW != 8) { env.error("CURSOR_RESIZE_EW should be 8"); return }
    if(window::CURSOR_RESIZE_NESW != 9) { env.error("CURSOR_RESIZE_NESW should be 9"); return }
    if(window::CURSOR_RESIZE_NWSE != 10) { env.error("CURSOR_RESIZE_NWSE should be 10"); return }
    if(window::CURSOR_NO != 11) { env.error("CURSOR_NO should be 11"); return }
    if(window::CURSOR_APPSTARTING != 12) { env.error("CURSOR_APPSTARTING should be 12"); return }
    if(window::CURSOR_SIZEALL != 13) { env.error("CURSOR_SIZEALL should be 13"); return }
}
