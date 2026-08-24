// window_practical_test.ch — Practical integration tests for the window library.
// These tests create real windows, manipulate them, and verify state changes.
// Each test runs in its own process (via @test + test_runner), so a GTK
// crash in one test won't bring down the whole suite.

using std::string;
using std::Result;

// ---------------------------------------------------------------------------
// Create a window and verify it's alive
// ---------------------------------------------------------------------------

@test
public func test_practical_window_create_and_destroy(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Test Create")
    w.width = 640
    w.height = 480
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) {
        // Headless CI — creation may fail, that's OK
        var Err(e) = res else unreachable
        env.error(e.message().data())
        return
    }
    if(!window::window_is_created(&raw mut w)) { env.error("window should be created"); return }
    if(window::window_native_handle(&raw mut w) == null) { env.error("native handle should not be null"); return }
    window::window_destroy(&raw mut w)
    if(window::window_is_created(&raw mut w)) { env.error("window should not be created after destroy"); return }
}

// ---------------------------------------------------------------------------
// Set title before creation, verify it's stored
// ---------------------------------------------------------------------------

@test
public func test_practical_window_title_roundtrip(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_title(&raw mut w, "My Title\0" as *char)
    var t = window::window_title(&raw mut w)
    // window_title returns a copy; check its content
    if(t.size() != 8) { env.error("title length should be 8"); return }
    if(t.get(0) != 'M' as char) { env.error("title should start with M"); return }
    if(t.get(1) != 'y' as char) { env.error("title[1] should be y"); return }
    if(t.get(7) != 'e' as char) { env.error("title[7] should be e"); return }
}

// ---------------------------------------------------------------------------
// Create window, change title, verify native title changed
// ---------------------------------------------------------------------------

@test
public func test_practical_window_set_title_after_create(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Before")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless
    if(!window::window_is_created(&raw mut w)) { env.error("should be created"); return }

    // Change title
    window::window_set_title(&raw mut w, "After\0" as *char)
    var t = window::window_title(&raw mut w)
    if(t.size() != 5) { env.error("title should be 'After' (len 5)"); return }
    if(t.get(0) != 'A' as char) { env.error("title should start with A"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Create window with specific size, verify stored size
// ---------------------------------------------------------------------------

@test
public func test_practical_window_size_after_create(env : &mut TestEnv) {
    var w = window::Window.make()
    w.width = 1024
    w.height = 768
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    var sz = window::window_size(&raw mut w)
    if(sz.width != 1024) { env.error("width should be 1024"); return }
    if(sz.height != 768) { env.error("height should be 768"); return }

    // Change size
    window::window_set_size(&raw mut w, 800, 600)
    var sz2 = window::window_size(&raw mut w)
    if(sz2.width != 800) { env.error("width should be 800 after set_size"); return }
    if(sz2.height != 600) { env.error("height should be 600 after set_size"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Create window, show it, verify visible state
// ---------------------------------------------------------------------------

@test
public func test_practical_window_show_hide(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("ShowHide Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_show(&raw mut w)
    if(!w.visible) { env.error("should be visible after show"); return }

    window::window_hide(&raw mut w)
    if(w.visible) { env.error("should not be visible after hide"); return }

    // Show again
    window::window_show(&raw mut w)
    if(!w.visible) { env.error("should be visible after second show"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Create window, show it, then destroy (no event loop needed)
// ---------------------------------------------------------------------------

@test
public func test_practical_window_show_and_destroy(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("ShowDestroy Test")
    w.width = 400
    w.height = 300
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_show(&raw mut w)
    if(!w.visible) { env.error("should be visible after show"); return }

    // Destroy without entering event loop
    window::window_destroy(&raw mut w)
    if(window::window_is_created(&raw mut w)) { env.error("should be destroyed"); return }
}

// ---------------------------------------------------------------------------
// Set and verify position before creation
// ---------------------------------------------------------------------------

@test
public func test_practical_window_position_before_create(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_position(&raw mut w, 100, 200)
    var pos = window::window_position(&raw mut w)
    if(pos.x != 100) { env.error("x should be 100"); return }
    if(pos.y != 200) { env.error("y should be 200"); return }
}

// ---------------------------------------------------------------------------
// Create window, set position, verify
// ---------------------------------------------------------------------------

@test
public func test_practical_window_position_after_create(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Position Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_set_position(&raw mut w, 50, 50)
    var pos = window::window_position(&raw mut w)
    // Position may differ from requested due to WM placement,
    // but it should be non-negative
    if(pos.x < 0) { env.error("x should be non-negative"); return }
    if(pos.y < 0) { env.error("y should be non-negative"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Set min/max constraints before creation
// ---------------------------------------------------------------------------

@test
public func test_practical_window_constraints(env : &mut TestEnv) {
    var w = window::Window.make()
    window::window_set_min_size(&raw mut w, 200, 150)
    window::window_set_max_size(&raw mut w, 1200, 900)
    var minsz = window::window_min_size(&raw mut w)
    var maxsz = window::window_max_size(&raw mut w)
    if(minsz.width != 200) { env.error("min width should be 200"); return }
    if(minsz.height != 150) { env.error("min height should be 150"); return }
    if(maxsz.width != 1200) { env.error("max width should be 1200"); return }
    if(maxsz.height != 900) { env.error("max height should be 900"); return }
}

// ---------------------------------------------------------------------------
// Create window, toggle always_on_top, verify
// ---------------------------------------------------------------------------

@test
public func test_practical_window_always_on_top(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Topmost Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_set_always_on_top(&raw mut w, true)
    if(!w.always_on_top) { env.error("always_on_top should be true"); return }

    window::window_set_always_on_top(&raw mut w, false)
    if(w.always_on_top) { env.error("always_on_top should be false"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Create window, change opacity, verify
// ---------------------------------------------------------------------------

@test
public func test_practical_window_opacity(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Opacity Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_set_opacity(&raw mut w, 0.5)
    if(w.opacity != 0.5) { env.error("opacity should be 0.5"); return }

    window::window_set_opacity(&raw mut w, 1.0)
    if(w.opacity != 1.0) { env.error("opacity should be 1.0"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Create window, change cursor, verify stored value
// ---------------------------------------------------------------------------

@test
public func test_practical_window_cursor_after_create(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Cursor Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_set_cursor(&raw mut w, window::CURSOR_HAND)
    if(w.cursor != window::CURSOR_HAND) { env.error("cursor should be CURSOR_HAND"); return }

    window::window_set_cursor(&raw mut w, window::CURSOR_IBEAM)
    if(w.cursor != window::CURSOR_IBEAM) { env.error("cursor should be CURSOR_IBEAM"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// DPI check after creation
// ---------------------------------------------------------------------------

@test
public func test_practical_window_dpi_after_create(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("DPI Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    var dpi = window::window_dpi(&raw mut w)
    if(dpi <= 0) { env.error("DPI should be positive"); return }
    var sf = window::window_scale_factor(&raw mut w)
    if(sf < 1.0) { env.error("scale factor should be >= 1.0"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Focus callback registration
// ---------------------------------------------------------------------------

@test
public func test_practical_window_focus_callback(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Focus CB Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    // Set a focus callback (we can't easily verify it fires without event loop,
    // but we verify it doesn't crash)
    window::window_set_focus_callback(&raw mut w, null)
    window::window_set_focus_callback(&raw mut w, null)

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Event callback registration
// ---------------------------------------------------------------------------

@test
public func test_practical_window_event_callback(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Event CB Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    // Set event callback to null — verify no crash
    window::window_set_event_callback(&raw mut w, null)

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Close callback registration
// ---------------------------------------------------------------------------

@test
public func test_practical_window_close_callback(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Close CB Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_set_close_callback(&raw mut w, null)

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// User data roundtrip
// ---------------------------------------------------------------------------

@test
public func test_practical_window_user_data(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("UserData Test")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    // Set user_data to some value
    var marker : int = 42
    window::window_set_user_data(&raw mut w, &raw mut marker as *mut void)
    // We can't read it back from the public API, but verify no crash

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Multiple windows
// ---------------------------------------------------------------------------

@test
public func test_practical_two_windows(env : &mut TestEnv) {
    var w1 = window::Window.make()
    w1.title = string("Window 1")
    w1.width = 400
    w1.height = 300
    var r1 = window::window_create(&raw mut w1)
    if(r1 is Result.Err) { return } // headless

    var w2 = window::Window.make()
    w2.title = string("Window 2")
    w2.width = 500
    w2.height = 400
    var r2 = window::window_create(&raw mut w2)
    if(r2 is Result.Err) {
        window::window_destroy(&raw mut w1)
        return
    }

    if(!window::window_is_created(&raw mut w1)) { env.error("w1 should be created"); return }
    if(!window::window_is_created(&raw mut w2)) { env.error("w2 should be created"); return }

    var sz1 = window::window_size(&raw mut w1)
    if(sz1.width != 400) { env.error("w1 width should be 400"); return }

    var sz2 = window::window_size(&raw mut w2)
    if(sz2.width != 500) { env.error("w2 width should be 500"); return }

    window::window_destroy(&raw mut w1)
    window::window_destroy(&raw mut w2)
}

// ---------------------------------------------------------------------------
// Monitor info (verify no crash, values are reasonable)
// ---------------------------------------------------------------------------

@test
public func test_practical_window_monitor_info(env : &mut TestEnv) {
    var count = window::window_monitor_count()
    if(count < 0) { env.error("monitor count should be non-negative"); return }
    if(count > 0) {
        var bounds = window::window_monitor_bounds(0)
        // Bounds should be non-negative
        if(bounds.x < 0) { env.error("monitor x should be non-negative"); return }
        if(bounds.y < 0) { env.error("monitor y should be non-negative"); return }
        if(bounds.width < 0) { env.error("monitor width should be non-negative"); return }
        if(bounds.height < 0) { env.error("monitor height should be non-negative"); return }
        var scale = window::window_monitor_scale(0)
        if(scale < 0.5) { env.error("monitor scale should be >= 0.5"); return }
    }
}

// ---------------------------------------------------------------------------
// Error: create null window
// ---------------------------------------------------------------------------

@test
public func test_practical_window_create_null_fails(env : &mut TestEnv) {
    var res = window::window_create(null)
    if(res is Result.Ok) { env.error("create(null) should fail"); return }
}

// ---------------------------------------------------------------------------
// Error: create already-created window
// ---------------------------------------------------------------------------

@test
public func test_practical_window_create_twice_fails(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Twice Test")
    var r1 = window::window_create(&raw mut w)
    if(r1 is Result.Err) { return } // headless

    var r2 = window::window_create(&raw mut w)
    if(r2 is Result.Ok) { env.error("creating twice should fail"); return }

    window::window_destroy(&raw mut w)
}

// ---------------------------------------------------------------------------
// Destroy is safe to call multiple times
// ---------------------------------------------------------------------------

@test
public func test_practical_window_destroy_twice(env : &mut TestEnv) {
    var w = window::Window.make()
    w.title = string("Destroy Twice")
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    window::window_destroy(&raw mut w)
    // Second destroy should not crash
    window::window_destroy(&raw mut w)
    if(window::window_is_created(&raw mut w)) { env.error("should not be created after double destroy"); return }
}

// ---------------------------------------------------------------------------
// Client size reflects physical pixels
// ---------------------------------------------------------------------------

@test
public func test_practical_window_client_size(env : &mut TestEnv) {
    var w = window::Window.make()
    w.width = 800
    w.height = 600
    var res = window::window_create(&raw mut w)
    if(res is Result.Err) { return } // headless

    var csz = window::window_client_size(&raw mut w)
    // Client size should be positive (physical pixels)
    if(csz.width <= 0) { env.error("client width should be > 0"); return }
    if(csz.height <= 0) { env.error("client height should be > 0"); return }

    window::window_destroy(&raw mut w)
}
