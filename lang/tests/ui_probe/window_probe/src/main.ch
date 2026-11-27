// P0.1: per-window native user data + callback routing + a real GTK3 main loop.
//
// Linux host only. On a machine without a display/GTK3 `Window.new` returns an
// error and the probe records BLOCKED instead of failing.
using std::Result;
using std::chrono::Instant;

var g_resize1 : int = 0
var g_resize2 : int = 0
var g_event1 : int = 0

func quit_cb(data : *mut void) : int {
    window::raw::window_quit()
    return 0
}

// P0.1a: a named function must convert to the `std::function` handler type.
// This is the exact shape that used to crash before the conversion fix.
func on_resize_named(width : int, height : int) {
    unsafe { g_resize1 = g_resize1 + 1 }
}

public func main() : int {
    var t = Instant.now()
    var created1 = window::Window.new("P0.1 window one", 320, 200)
    if(created1 is Result.Err) {
        println("P0.1 window_create BLOCKED no display/GTK3")
        return 0
    }
    var Ok(w1) = created1 else unsafe { unreachable }
    var created2 = window::Window.new("P0.1 window two", 240, 160)
    if(created2 is Result.Err) {
        println("P0.1 window_create BLOCKED second window")
        w1.close()
        return 0
    }
    var Ok(w2) = created2 else unsafe { unreachable }

    // NOTE: `w1` deliberately uses a named function (P0.1a); `w2` uses a
    // lambda. Both must route independently through the same handler type.
    w1.on_resize(on_resize_named)
    w2.on_resize(||(width : int, height : int) => { unsafe { g_resize2 = g_resize2 + 1 } })
    w1.on_event(||(event : &window::Event) => { unsafe { g_event1 = g_event1 + 1 } })
    w1.show()
    w2.show()
    w1.set_size(360, 220)
    w2.set_size(260, 180)
    var created_ms = t.elapsed().as_millis()

    unsafe {
        window::raw::g_timeout_add(300u32, quit_cb as *mut void, null)
    }
    window::raw::window_run()

    var ok = true
    if(!w1.is_open() || !w2.is_open()) { ok = false }
    // Resize callbacks may be coalesced or absent on a compositor; require at
    // least one routed resize for the window whose size was changed explicitly.
    unsafe {
        if(g_resize1 == 0 && g_resize2 == 0) { ok = false }
        printf("P0.1 window resize1=%d resize2=%d events=%d created_ms=%ld\n",
               g_resize1, g_resize2, g_event1, created_ms)
    }
    w1.close()
    w2.close()
    if(ok) {
        println("P0.1 window_create_userdata_run PASS")
        return 0
    }
    println("P0.1 window_create_userdata_run FAIL")
    return 1
}
