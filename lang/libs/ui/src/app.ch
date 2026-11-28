// P1.10 headless app loop. No window, input, focus, IME, or multi-window.
// The public surface implements the API.md app/options/context subset needed
// for headless rendering; P1.11+ extends UiCtx.
using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;
using ui_internal::HitResult;

public enum Renderer { Auto, Cpu, Gpu }
public enum GpuBackend { Auto, Vulkan, Metal, D3D12, WebGpu, OpenGl }
public enum GpuPreference { Auto, Discrete, Integrated, Software }

@direct_init
public struct FontFallback { var script : u32; var families : &[string_view] }
@direct_init
public struct FontStack {
    var ui : &[string_view]
    var mono : &[string_view]
    var fallback : &[FontFallback]
    @make func system() : FontStack {
        var empty_sv : [string_view] = unsafe(std::span<string_view>(null, 0u))
        var empty_fb : [FontFallback] = unsafe(std::span<FontFallback>(null, 0u))
        return FontStack { ui : empty_sv, mono : empty_sv, fallback : empty_fb }
    }
}

@direct_init
public struct Limits {
    var max_view_nodes : u32
    var max_retained_nodes : u32
    var max_text_bytes : u64
    var max_image_pixels : u64
    var max_image_bytes : u64
    var max_font_bytes : u64
    var max_atlas_bytes : u64
    var max_layer_bytes : u64
    var max_gpu_bytes : u64
    var max_display_commands : u32
    var max_protocol_bytes : u32
    var max_capture_frames : u32
    var max_screenshot_pixels : u64
    @make func make() : Limits {
        return Limits {
            max_view_nodes : 8192u32, max_retained_nodes : 8192u32,
            max_text_bytes : 16u64 * 1024u64 * 1024u64,
            max_image_pixels : 16u64 * 1024u64 * 1024u64,
            max_image_bytes : 256u64 * 1024u64 * 1024u64,
            max_font_bytes : 64u64 * 1024u64 * 1024u64,
            max_atlas_bytes : 64u64 * 1024u64 * 1024u64,
            max_layer_bytes : 64u64 * 1024u64 * 1024u64,
            max_gpu_bytes : 256u64 * 1024u64 * 1024u64,
            max_display_commands : 65536u32, max_protocol_bytes : 16u32 * 1024u32 * 1024u32,
            max_capture_frames : 16u32, max_screenshot_pixels : 16u64 * 1024u64 * 1024u64
        }
    }
}

@direct_init
public struct FrameStats {
    var frame : u64
    var renderer : Renderer
    var gpu_backend : GpuBackend
    var build_us : double
    var layout_us : double
    var paint_us : double
    var gpu_us : double
    var present_us : double
    var event_to_update_us : double
    var update_to_submit_us : double
    var submit_to_present_us : double
    var nodes : u32
    var visited : u32
    var relaid_out : u32
    var layout_nodes : u32
    var virtual_rebuilds : u32
    var virtual_extent_calls : u32
    var virtual_index_builds : u32
    var commands : u32
    var glyphs : u32
    var text_measured : u32
    var text_cache_hits : u32
    var tiles_painted : u32
    var full_repaints : u32
    var paint_bytes : u64
    var scroll_blit_bytes : u64
    var scroll_blit_us : double
    var allocs : u32
    var allocs_cold : u32
    var alloc_bytes_high_water : u64
    var settle_frames : u32
    var gpu_upload_bytes : u64
    var gpu_draw_calls : u32
}

@direct_init
public struct Screenshot {
    var path : string_view
    var width : u32
    var height : u32
    var hash : u64
}

public enum EventKind {
    Click, DoubleClick, ContextMenu, Press, Release, PointerMove, PointerEnter,
    PointerLeave, Scroll, KeyDown, KeyUp, TextInput, Focus, Blur, Frame, Command,
    Custom, Resize, DpiChange, Timer
}

public enum KeyCode {
    Unknown, Tab, Enter, Escape, Space, Backspace, Delete, Insert,
    Left, Right, Up, Down, Home, End, PageUp, PageDown,
    A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,
    Digit0,Digit1,Digit2,Digit3,Digit4,Digit5,Digit6,Digit7,Digit8,Digit9,
    Minus,Equal,LeftBracket,RightBracket,Backslash,Semicolon,Quote,Comma,Period,Slash,Backquote,
    F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,F11,F12,F13,F14,F15,F16,F17,F18,F19,F20,F21,F22,F23,F24,
    ShiftLeft,ShiftRight,ControlLeft,ControlRight,AltLeft,AltRight,MetaLeft,MetaRight,CapsLock,
    Numpad0,Numpad1,Numpad2,Numpad3,Numpad4,Numpad5,Numpad6,Numpad7,Numpad8,Numpad9,
    NumpadAdd,NumpadSubtract,NumpadMultiply,NumpadDivide,NumpadDecimal,NumpadEnter,
    PrintScreen,ScrollLock,Pause,ContextMenu,MediaPlayPause,MediaStop,MediaNext,MediaPrev,
    VolumeUp,VolumeDown,VolumeMute
}

@direct_init
public struct Modifiers { var shift : bool; var ctrl : bool; var alt : bool; var meta : bool; var caps : bool }
@direct_init
public struct PointerPayload {
    var button : u8; var press_count : u8; var pointer_kind : u8
    var pressure : float; var tilt_x : float; var tilt_y : float
}
@direct_init
public struct ScrollPayload { var dx : float; var dy : float; var phase : u8 }
@direct_init
public struct KeyPayload { var code : KeyCode; var repeat : bool; var text : Option<string_view> }
@direct_init
public struct TextPayload { var text : string_view }
@direct_init
public struct FocusPayload { var focused : bool }

public variant Payload {
    None()
    Pointer(value : PointerPayload)
    Scroll(value : ScrollPayload)
    Key(value : KeyPayload)
    Text(value : TextPayload)
    Focus(value : FocusPayload)
}

@direct_init
public struct UiEvent {
    var window : WindowId
    var key : Key
    var kind : EventKind
    var payload : Payload
    var modifiers : Modifiers
    var local_x : float
    var local_y : float
    var timestamp_ms : double
    var frame : u64
    // Direct normalized fields mirror Payload for stable ABI dispatch.
    var pointer_button : u8
    var scroll_dx : float
    var scroll_dy : float
    var key_code : KeyCode
    var key_repeat : bool
    var text_value : string_view
}

public enum CloseAction { Allow, Veto }

public struct Options {
    var title : string_view = "Chemical"
    var width : i32 = 960
    var height : i32 = 640
    var min_width : i32 = 320
    var min_height : i32 = 240
    var theme : ThemeMode = ThemeMode.System
    var scale : float = 0.0f
    var headless : bool = false
    var debug : bool = false
    var debug_port : u16 = 0
    var fixed_time : bool = false
    var reduced_motion : bool = false
    var vsync : bool = true
    var max_frames : u32 = 0
    var limits : Limits
    var theme_override : Option<Theme> = Option.None<Theme>()
    var locale : string_view = "en-US"
    var max_fps : u8 = 0
    var reveal_secrets : bool = false
    var renderer : Renderer = Renderer.Auto
    var gpu_backend : GpuBackend = GpuBackend.Auto
    var gpu_prefer : GpuPreference = GpuPreference.Auto
    var allow_software_gpu : bool = false
    var gpu_frames_in_flight : u8 = 2
    var font_stack : FontStack
    var font_file : string_view = ""
    var font_cache : string_view = ""
    var font_size_scale : float = 1.0f
    @make func make() : Options {
        return Options {
            title : "Chemical", width : 960, height : 640,
            min_width : 320, min_height : 240, theme : ThemeMode.System,
            scale : 0.0f, headless : false, debug : false, debug_port : 0u16,
            fixed_time : false, reduced_motion : false, vsync : true,
            max_frames : 0u32, limits : Limits.make(),
            theme_override : Option.None<Theme>(), locale : "en-US",
            max_fps : 0u8, reveal_secrets : false, renderer : Renderer.Auto,
            gpu_backend : GpuBackend.Auto, gpu_prefer : GpuPreference.Auto,
            allow_software_gpu : false, gpu_frames_in_flight : 2u8,
            font_stack : FontStack.system(), font_file : "", font_cache : "",
            font_size_scale : 1.0f
        }
    }
}

public interface Application {
    func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit, UiError>
    func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId, UiError>
    func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit, UiError>
    func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit, UiError>
    func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction
}

public struct HeadlessState {
    var options : Options
    var frame : FrameInfo
    var viewport : Viewport
    var theme : Theme
    var layout_ctx : ui_internal::LayoutContext
    var builder : ViewBuilder
    var retained : ui_internal::RetainedTree
    var layout : ui_internal::LayoutState
    var display : ui_internal::DisplayState
    var buffer : ui_internal::ColorBuffer
    var stats : FrameStats
    var focus_key : Option<Key>
    var hover_key : Option<Key>
    var pressed_key : Option<Key>
    var capture_key : Option<Key>
    var last_click_key : Option<Key>
    var last_click_ms : double
    var last_error : Option<UiError>
    var error_active : bool
    var error_retries : u32
    var focus_before_error : Option<Key>
    var dirty : bool
    var quit : bool
    var frame_no : u64
    var frames_run : u32
    var view_calls : u32
    var buffer_ready : bool
}

public struct UiCtx {
    internal var frame_value : FrameInfo
    internal var viewport_value : Viewport
    internal var theme_value : Theme
    internal var textctx_value : TextContext
    internal var stats_value : FrameStats
    internal var dirty_value : bool
    internal var quit_value : bool
    internal var text_scale_value : float
    internal var focus_value : Option<Key>
    internal var reported_error_value : Option<UiError>
    internal var event_kind_value : EventKind
    internal var event_key_value : Key
    internal var event_code_value : KeyCode
    internal var event_repeat_value : bool
    internal var event_shift_value : bool
    internal var event_text_value : string_view
    internal var event_scroll_value : Point
    internal var scroll_key_value : Key
    internal var scroll_point_value : Point
    internal var has_scroll_value : bool

    public func frame_info(&self) : &FrameInfo { return &self.frame_value }
    public func viewport(&self) : Viewport { return self.viewport_value }
    public func scale(&self) : float { return self.frame_value.scale }
    public func text_scale(&self) : float { return self.text_scale_value }
    public func text_context(&self) : &TextContext { return &self.textctx_value }
    public func theme(&self) : &Theme { return &self.theme_value }
    public func theme_mode(&self) : ThemeMode { return self.frame_value.theme }
    public func focused_key(&self) : Option<Key> { return self.focus_value }
    public func event_kind(&self) : EventKind { return self.event_kind_value }
    public func event_code(&self) : KeyCode { return self.event_code_value }
    public func event_repeat(&self) : bool { return self.event_repeat_value }
    public func event_shift(&self) : bool { return self.event_shift_value }
    public func event_text(&self) : string_view { return self.event_text_value }
    public func event_scroll(&self) : Point { return self.event_scroll_value }
    public func is_focused(&self, key : Key) : bool {
        if(self.focus_value is Option.Some) { var Some(k) = self.focus_value else return false; return k.equals_key(key) }
        return false
    }
    public func focus(&mut self, key : Key) { self.focus_value = Option.Some<Key>(key) }
    public func scroll_offset(&self, key : Key) : Point {
        if(self.has_scroll_value && self.scroll_key_value.equals_key(key)) { return self.scroll_point_value }
        return Point.zero()
    }
    public func set_scroll(&mut self, key : Key, offset : Point) {
        self.scroll_key_value = key; self.scroll_point_value = offset; self.has_scroll_value = true
    }
    public func scroll_to(&mut self, key : Key) { self.scroll_key_value = key; self.has_scroll_value = true; self.scroll_point_value = Point.zero() }
    public func now_ms(&self) : double { return self.frame_value.now_ms }
    public func frame(&self) : u64 { return self.frame_value.frame }
    public func stats(&self) : FrameStats { return self.stats_value }
    public func request_view(&mut self) { self.dirty_value = true }
    public func invalidate(&mut self, key : Key) { self.dirty_value = true }
    public func request_paint(&mut self) {}
    public func request_animation_frame(&mut self) {}
    public func request_frame(&mut self) {}
    public func request_quit(&mut self) { self.quit_value = true }
    public func report_error(&mut self, key : Key, err : UiError) { self.reported_error_value = Option.Some<UiError>(err) }
    public func set_theme(&mut self, theme : Theme) { self.theme_value = theme; self.dirty_value = true }
    public func set_theme_mode(&mut self, mode : ThemeMode) { self.frame_value.theme = mode; self.dirty_value = true }
    public func set_text_scale(&mut self, scale : float) {
        if(scale > 0.0f && scale == scale && finite32(scale)) { self.text_scale_value = scale; self.dirty_value = true }
    }
}

@thread_local @never_destructed
var g_headless_events : vector<UiEvent> = zeroed:unsafe<vector<UiEvent>>()
@thread_local @never_destructed
var g_headless_event_max_us : double = 0.0

@retained
public func headless_queue_size() : u32 { unsafe { return g_headless_events.size() as u32 } }
@retained
public func headless_queue_at(i : size_t) : UiEvent { unsafe { return g_headless_events.get_ptr(i)[0] } }
@retained
public func headless_queue_clear() { unsafe { g_headless_events.clear() } }
@retained
public func headless_option_key(o : Option<Key>, fallback : Key) : Key {
    switch(o) {
        Some(v) => { return v }
        None() => { }
    }
    return fallback
}
@retained
public func headless_option_hit(o : Option<HitResult>, fallback : HitResult) : HitResult {
    switch(o) {
        Some(v) => { return v }
        None() => { }
    }
    return fallback
}
@retained
public func headless_event_scroll_payload(ev : &UiEvent) : ScrollPayload {
    switch(ev.payload) {
        Scroll(v) => { return v }
        default => { }
    }
    return ScrollPayload { dx : 0.0f, dy : 0.0f, phase : 1u8 }
}
@retained
public func headless_event_key_payload(ev : &UiEvent) : KeyPayload {
    switch(ev.payload) {
        Key(v) => { return v }
        default => { }
    }
    return KeyPayload { code : KeyCode.Unknown, repeat : false, text : Option.None<string_view>() }
}

@retained
public func headless_hit_current(state : &HeadlessState, p : Point) : Option<HitResult> {
    return ui_internal::hit_test(&state.display, &state.layout, p)
}
@retained
public func headless_hit_focusable_current(state : &HeadlessState, p : Point) : Option<HitResult> {
    return ui_internal::hit_test_focusable(&state.display, &state.layout, p)
}
@retained
public func headless_focus_next_current(state : &HeadlessState, current : Option<Key>, backwards : bool) : Option<HitResult> {
    return ui_internal::focus_next(&state.display, current, backwards)
}
@retained
public func headless_scroll_current(state : &mut HeadlessState, hit : HitResult, dx : float, dy : float) : Option<Key> {
    return ui_internal::scroll_by_hit(&mut state.layout, &hit, dx, dy)
}

@retained
public func headless_event_base(kind : EventKind, x : float, y : float, mods : Modifiers) : UiEvent {
    return UiEvent {
        window : zeroed<WindowId>(), key : zeroed<Key>(), kind : kind,
        payload : Payload.None(), modifiers : mods, local_x : x, local_y : y,
        timestamp_ms : 0.0, frame : 0u64,
        pointer_button : 0u8, scroll_dx : 0.0f, scroll_dy : 0.0f,
        key_code : KeyCode.Unknown, key_repeat : false, text_value : ""
    }
}

@retained
public func headless_event_max_us() : double { unsafe { return g_headless_event_max_us } }
@retained
public func headless_event_reset_timing() { unsafe { g_headless_event_max_us = 0.0 } }
@retained
public func headless_input_reset() { headless_queue_clear() }
@retained
public func headless_input_count() : u32 { return headless_queue_size() }
@retained
public func headless_input_pointer(kind : EventKind, x : float, y : float, button : u8, mods : Modifiers) {
    var e = headless_event_base(kind, x, y, mods)
    e.pointer_button = button
    e.payload = Payload.Pointer(PointerPayload {
        button : button, press_count : 1u8, pointer_kind : 1u8,
        pressure : 1.0f, tilt_x : 0.0f, tilt_y : 0.0f
    })
    unsafe { g_headless_events.push(e) }
}
@retained
public func headless_input_pointer_at(kind : EventKind, x : float, y : float, button : u8, mods : Modifiers, at_ms : double) {
    var e = headless_event_base(kind, x, y, mods)
    e.timestamp_ms = at_ms
    e.pointer_button = button
    e.payload = Payload.Pointer(PointerPayload {
        button : button, press_count : 1u8, pointer_kind : 1u8,
        pressure : 1.0f, tilt_x : 0.0f, tilt_y : 0.0f
    })
    unsafe { g_headless_events.push(e) }
}
@retained
public func headless_input_wheel(x : float, y : float, dx : float, dy : float, mods : Modifiers) {
    var e = headless_event_base(EventKind.Scroll, x, y, mods)
    e.scroll_dx = dx; e.scroll_dy = dy
    e.payload = Payload.Scroll(ScrollPayload { dx : dx, dy : dy, phase : 1u8 })
    unsafe { g_headless_events.push(e) }
}
@retained
public func headless_input_key(kind : EventKind, code : KeyCode, mods : Modifiers, repeat : bool, text : string_view) {
    var e = headless_event_base(kind, 0.0f, 0.0f, mods)
    var text_opt = Option.None<string_view>()
    if(text.size() > 0u) { text_opt = Option.Some<string_view>(text) }
    e.key_code = code; e.key_repeat = repeat; e.text_value = text
    e.payload = Payload.Key(KeyPayload { code : code, repeat : repeat, text : text_opt })
    unsafe { g_headless_events.push(e) }
}
@retained
public func headless_input_text(text : string_view) {
    var e = headless_event_base(EventKind.TextInput, 0.0f, 0.0f, Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false })
    e.text_value = text
    e.payload = Payload.Text(TextPayload { text : text })
    unsafe { g_headless_events.push(e) }
}

@retained
public func <A> headless_call_update(app : &mut A, state : &mut HeadlessState, ctx : &mut UiCtx, ev : &UiEvent) : Result<Unit,UiError> where A : Application {
    ctx.focus_value = state.focus_key
    ctx.stats_value = state.stats
    ctx.event_kind_value = ev.kind
    ctx.event_key_value = ev.key
    ctx.event_code_value = ev.key_code
    ctx.event_repeat_value = ev.key_repeat
    ctx.event_shift_value = ev.modifiers.shift
    ctx.event_text_value = ev.text_value
    ctx.event_scroll_value = Point { x : ev.scroll_dx, y : ev.scroll_dy }
    var uf = headless_fault_take(3u8)
    if(uf is Option.Some) {
        var fe = headless_option_error(uf, UiError.make(ErrorCode.Internal, "update fault corrupt"))
        return Result.Err<Unit,UiError>(fe)
    }
    var t = std::chrono::Instant.now()
    var r = app.update(ev, ctx)
    var us = (t.elapsed().as_nanos() as double) / 1000.0
    state.stats.event_to_update_us = us
    unsafe { if(us > g_headless_event_max_us) { g_headless_event_max_us = us } }
    state.dirty = ctx.dirty_value
    state.quit = ctx.quit_value
    state.theme = ctx.theme_value
    state.frame.theme = ctx.frame_value.theme
    state.focus_key = ctx.focus_value
    var reported = headless_report_take(ctx)
    if(reported.is_some()) { headless_set_error(state, headless_option_error(reported, UiError.make(ErrorCode.Internal, "reported error corrupt"))) }
    if(ctx.has_scroll_value) {
        ui_internal::layout_set_scroll(&mut state.layout, ctx.scroll_key_value, ctx.scroll_point_value)
        state.dirty = true
    }
    return r
}

@retained
public func <A> headless_set_focus(app : &mut A, state : &mut HeadlessState, ctx : &mut UiCtx, next : Option<Key>) : Result<Unit,UiError> where A : Application {
    var old_some = state.focus_key.is_some()
    var old_key = headless_option_key(state.focus_key, zeroed<Key>())
    var next_some = next.is_some()
    var next_key = headless_option_key(next, zeroed<Key>())
    var same = false
    if(old_some && next_some) { same = old_key.equals_key(next_key) }
    else if(!old_some && !next_some) { same = true }
    if(same) { return Result.Ok<Unit,UiError>(Unit{}) }
    if(old_some) {
        var be = headless_event_base(EventKind.Blur, 0.0f, 0.0f, Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false })
        be.key = old_key
        be.payload = Payload.Focus(FocusPayload { focused : false })
        var br = headless_call_update(app, state, ctx, &be)
        if(br is Result.Err) { var Err(e) = br else return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Internal, "blur result corrupt")); return Result.Err<Unit,UiError>(e) }
    }
    state.focus_key = next
    if(next_some) {
        var fe = headless_event_base(EventKind.Focus, 0.0f, 0.0f, Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false })
        fe.key = next_key
        fe.payload = Payload.Focus(FocusPayload { focused : true })
        var fr = headless_call_update(app, state, ctx, &fe)
        if(fr is Result.Err) { var Err(e) = fr else return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Internal, "focus result corrupt")); return Result.Err<Unit,UiError>(e) }
    }
    return Result.Ok<Unit,UiError>(Unit{})
}

@retained
public func <A> headless_dispatch_events(app : &mut A, state : &mut HeadlessState, ctx : &mut UiCtx) : Result<u32, UiError> where A : Application {
    if(headless_queue_size() == 0u32) { return Result.Ok<u32,UiError>(0u32) }
    var count = 0u32
    var i : size_t = 0u
    while(i < (headless_queue_size() as size_t)) {
        var ev = headless_queue_at(i)
        var target = headless_hit_current(state, Point { x : ev.local_x, y : ev.local_y })
        var target_some = target.is_some()
        var target_hit = headless_option_hit(target, zeroed<HitResult>())
        var target_key = target_hit.key
        var focus_target = headless_hit_focusable_current(state, Point { x : ev.local_x, y : ev.local_y })
        var focus_some = focus_target.is_some()
        var focus_hit = headless_option_hit(focus_target, zeroed<HitResult>())
        if(state.error_active) {
            if(target_some && target_key.equals_key(headless_error_retry_key()) && (ev.kind == EventKind.Release || ev.kind == EventKind.Click)) {
                state.error_active = false
                state.last_error = Option.None<UiError>()
                state.focus_key = state.focus_before_error
                state.error_retries = state.error_retries + 1u32
                state.dirty = true
                count = count + 1u32
            }
            i = i + 1u
            continue
        }
        if(ev.kind == EventKind.PointerMove || ev.kind == EventKind.PointerEnter) {
            var old_some = state.hover_key.is_some()
            var old_hover = headless_option_key(state.hover_key, zeroed<Key>())
            var hov = Option.None<Key>()
            if(target_some) { hov = Option.Some<Key>(target_key) }
            state.hover_key = hov
            if(old_some && !old_hover.equals_key(target_key)) {
                var le = headless_event_base(EventKind.PointerLeave, ev.local_x, ev.local_y, ev.modifiers)
                le.key = old_hover
                var lr = headless_call_update(app, state, ctx, &le)
                if(lr is Result.Err) { var Err(e) = lr else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "leave corrupt")); return Result.Err<u32,UiError>(e) }
            }
            ev.key = target_key
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "move corrupt")); return Result.Err<u32,UiError>(e) }
            if(target_some) {
                var en = headless_event_base(EventKind.PointerEnter, ev.local_x, ev.local_y, ev.modifiers)
                en.key = target_key
                var er = headless_call_update(app, state, ctx, &en)
                if(er is Result.Err) { var Err(e) = er else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "enter corrupt")); return Result.Err<u32,UiError>(e) }
            }
        } else if(ev.kind == EventKind.PointerLeave) {
            state.hover_key = Option.None<Key>()
            ev.key = zeroed<Key>()
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "leave corrupt")); return Result.Err<u32,UiError>(e) }
        } else if(ev.kind == EventKind.Press || ev.kind == EventKind.ContextMenu) {
            ev.key = target_key
            if(focus_some) {
                var fr = headless_set_focus(app, state, ctx, Option.Some<Key>(focus_hit.key))
                if(fr is Result.Err) { var Err(e) = fr else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "focus corrupt")); return Result.Err<u32,UiError>(e) }
            }
            if(ev.kind == EventKind.Press) {
                var pkey = Option.None<Key>()
                if(target_some) { pkey = Option.Some<Key>(target_key) }
                state.pressed_key = pkey
                state.capture_key = state.pressed_key
            }
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "press corrupt")); return Result.Err<u32,UiError>(e) }
        } else if(ev.kind == EventKind.Release) {
            var pressed_some = state.pressed_key.is_some()
            var pressed_key = headless_option_key(state.pressed_key, zeroed<Key>())
            var release_key = target_key
            if(pressed_some) { release_key = pressed_key }
            ev.key = release_key
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "release corrupt")); return Result.Err<u32,UiError>(e) }
            var release_same = target_some && target_key.equals_key(pressed_key)
            if(pressed_some && release_same) {
                var ce = headless_event_base(EventKind.Click, ev.local_x, ev.local_y, ev.modifiers)
                ce.key = release_key
                ce.payload = ev.payload
                var when = ev.timestamp_ms
                if(when == 0.0) { when = state.frame.now_ms }
                var last_some = state.last_click_key.is_some()
                var last_key = headless_option_key(state.last_click_key, zeroed<Key>())
                var was_double = last_some && last_key.equals_key(release_key) && (when - state.last_click_ms) <= 400.0
                if(was_double) { ce.kind = EventKind.DoubleClick }
                state.last_click_key = Option.Some<Key>(release_key)
                state.last_click_ms = when
                var cr = headless_call_update(app, state, ctx, &ce)
                if(cr is Result.Err) { var Err(e) = cr else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "click corrupt")); return Result.Err<u32,UiError>(e) }
            }
            state.pressed_key = Option.None<Key>()
            state.capture_key = Option.None<Key>()
        } else if(ev.kind == EventKind.Scroll) {
            var sd = ScrollPayload { dx : ev.scroll_dx, dy : ev.scroll_dy, phase : 1u8 }
            var key_for_scroll = target_key
            if(target_some) {
                var got = headless_scroll_current(state, target_hit, sd.dx, sd.dy)
                if(got.is_some()) { key_for_scroll = headless_option_key(got, target_key) }
            }
            ev.key = key_for_scroll
            var soff = ui_internal::layout_scroll_offset(&state.layout, key_for_scroll)
            ctx.scroll_key_value = key_for_scroll
            ctx.scroll_point_value = soff
            ctx.has_scroll_value = true
            state.dirty = true
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "scroll corrupt")); return Result.Err<u32,UiError>(e) }
        } else if(ev.kind == EventKind.KeyDown || ev.kind == EventKind.KeyUp) {
            var kp = KeyPayload { code : ev.key_code, repeat : ev.key_repeat, text : Option.None<string_view>() }
            var focus_some_key = state.focus_key.is_some()
            var evkey = headless_option_key(state.focus_key, zeroed<Key>())
            ev.key = evkey
            if(ev.kind == EventKind.KeyDown && kp.code == KeyCode.Tab) {
                var nf = headless_focus_next_current(state, state.focus_key, ev.modifiers.shift)
                var nk = Option.None<Key>()
                if(nf.is_some()) { nk = Option.Some<Key>(headless_option_hit(nf, zeroed<HitResult>()).key) }
                var fr = headless_set_focus(app, state, ctx, nk)
                if(fr is Result.Err) { var Err(e) = fr else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "tab corrupt")); return Result.Err<u32,UiError>(e) }
            } else if(ev.kind == EventKind.KeyDown && kp.code == KeyCode.Escape) {
                var fr = headless_set_focus(app, state, ctx, Option.None<Key>())
                if(fr is Result.Err) { var Err(e) = fr else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "escape corrupt")); return Result.Err<u32,UiError>(e) }
            }
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "key corrupt")); return Result.Err<u32,UiError>(e) }
        } else if(ev.kind == EventKind.TextInput) {
            var fk = headless_option_key(state.focus_key, zeroed<Key>())
            ev.key = fk
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "text corrupt")); return Result.Err<u32,UiError>(e) }
        } else {
            var r = headless_call_update(app, state, ctx, &ev)
            if(r is Result.Err) { var Err(e) = r else return Result.Err<u32,UiError>(UiError.make(ErrorCode.Internal, "event corrupt")); return Result.Err<u32,UiError>(e) }
        }
        count = count + 1u32
        if(state.quit) { break }
        i = i + 1u
    }
    headless_queue_clear()
    return Result.Ok<u32,UiError>(count)
}

@retained
public func headless_report_take(ctx : &mut UiCtx) : Option<UiError> {
    var o = ctx.reported_error_value
    ctx.reported_error_value = Option.None<UiError>()
    return o
}
@retained
public func headless_result_error(r : Result<Unit,UiError>, fallback : UiError) : UiError {
    switch(r) {
        Err(e) => { return e }
        Ok(v) => { return fallback }
    }
}
@retained
public func headless_option_error(o : Option<UiError>, fallback : UiError) : UiError {
    switch(o) {
        Some(e) => { return e }
        None() => { }
    }
    return fallback
}
@retained
public func <A> headless_paint_node(app : &A, state : &mut HeadlessState, index : u32, scale : float) : Option<UiError> where A : Application {
    if(index == 0u32) { return Option.None<UiError>() }
    var n = unsafe(state.layout.result.nodes.get_ptr(index as size_t))
    var id = unsafe(n.id)
    var kindo = ui_internal::view_node_kind(&state.builder, id)
    if(kindo.is_some()) {
        var kind = headless_option_kind(kindo, ui_internal::ViewKind.Root)
        if(kind == ui_internal::ViewKind.Canvas) {
            var cvo = ui_internal::view_canvas_id(&state.builder, id)
            var rct = ui_internal::layout_rect(&state.layout, id)
            if(cvo.is_some() && rct.is_some()) {
                var cv = headless_option_canvas(cvo, zeroed<CanvasId>())
                var rect = headless_option_rect(rct, Rect.zero())
                var pf = headless_fault_take(6u8)
                if(pf.is_some()) { return pf }
                var painter = ui_internal::new_painter(&raw mut state.display.list, &raw mut state.theme, Size.new(state.viewport.size.width, state.viewport.size.height))
                var cp = CanvasPaint { node : id, key : unsafe(n.key), id : cv, bounds : rect, scale : scale }
                var pr = app.paint(&state.frame, cp, &mut painter)
                if(pr is Result.Err) {
                    return Option.Some<UiError>(headless_result_error(pr, UiError.make(ErrorCode.Internal, "paint result corrupt")))
                }
            }
        }
    }
    var cc = unsafe(n.child_count)
    var i = 0u32
    while(i < cc) {
        var cho = ui_internal::layout_child_at(&state.layout, index, i)
        if(cho.is_some()) {
            var ci = headless_option_u32(cho, 0u32)
            var e = headless_paint_node(app, state, ci, scale)
            if(e.is_some()) { return e }
        }
        i = i + 1u32
    }
    return Option.None<UiError>()
}

@retained
public func <A> headless_paint_canvases(app : &A, state : &mut HeadlessState, scale : float) : Option<UiError> where A : Application {
    return headless_paint_node(app, state, ui_internal::layout_root(&state.layout), scale)
}

@retained
public func headless_option_u32(o : Option<u32>, fallback : u32) : u32 { switch(o) { Some(v) => { return v } None() => { } }; return fallback }
@retained
public func headless_option_kind(o : Option<ui_internal::ViewKind>, fallback : ui_internal::ViewKind) : ui_internal::ViewKind { switch(o) { Some(v) => { return v } None() => { } }; return fallback }
@retained
public func headless_option_canvas(o : Option<CanvasId>, fallback : CanvasId) : CanvasId { switch(o) { Some(v) => { return v } None() => { } }; return fallback }
@retained
public func headless_option_rect(o : Option<Rect>, fallback : Rect) : Rect { switch(o) { Some(v) => { return v } None() => { } }; return fallback }

@retained
public func headless_error_key() : Key { return key("__ui_error_card") }
@retained
public func headless_error_retry_key() : Key { return key("__ui_error_retry") }

@thread_local @never_destructed
var g_fault_where : u8 = 0u8
@thread_local @never_destructed
var g_fault_code : ErrorCode = ErrorCode.Other
@thread_local @never_destructed
var g_fault_remaining : u32 = 0u32

@retained
public func headless_fault_set(site : u8, code : ErrorCode, count : u32) {
    unsafe { g_fault_where = site; g_fault_code = code; g_fault_remaining = count }
}
@retained
public func headless_fault_clear() { unsafe { g_fault_where = 0u8; g_fault_remaining = 0u32 } }
@retained
public func headless_fault_remaining() : u32 { unsafe { return g_fault_remaining } }

@retained
public func headless_fault_take(site : u8) : Option<UiError> {
    unsafe {
        if(g_fault_where == site && g_fault_remaining > 0u32) {
            g_fault_remaining = g_fault_remaining - 1u32
            return Option.Some<UiError>(UiError.make(g_fault_code, "injected runtime fault"))
        }
    }
    return Option.None<UiError>()
}

@retained
public func headless_set_error(state : &mut HeadlessState, err : UiError) {
    if(!state.error_active) {
        state.focus_before_error = state.focus_key
        state.error_retries = 0u32
    }
    state.last_error = Option.Some<UiError>(err)
    state.error_active = true
    state.focus_key = Option.Some<Key>(headless_error_retry_key())
    state.dirty = true
}

@retained
public func headless_error_message(state : &HeadlessState) : string_view {
    switch(state.last_error) {
        Some(e) => { return e.detail().message.to_view() }
        None() => { }
    }
    return "UI error"
}

@retained
public func headless_build_error_card(state : &mut HeadlessState) {
    ui_internal::reset_view_builder(&mut state.builder)
    var root = ui_internal::view_root(&state.builder)
    state.builder.style(root, Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
    var card = state.builder.column(key("__ui_error_card"), Style.new().width(Dim.Px(300.0f)).height(Dim.Px(160.0f)).padding_all(Space.MD).background(ColorToken.SurfaceRaised).border(Border { width : 2.0f, color : ColorToken.Danger }).radius(Radius.MD))
    state.builder.test_id(card, "error-card")
    var cs = ui_internal::semantics_empty(); cs.role = Role.Alert; cs.label = "UI error"; cs.description = headless_error_message(state)
    state.builder.semantics(card, cs)
    state.builder.child(root, card)
    var msg = state.builder.text(key("__ui_error_message"), headless_error_message(state), Style.new().text(TextRole.Body).width(Dim.Px(260.0f)).height(Dim.Px(24.0f))); state.builder.child(card, msg)
    var retry = state.builder.row(key("__ui_error_retry"), Style.new().width(Dim.Px(120.0f)).height(Dim.Px(44.0f)).background(ColorToken.Primary).foreground(ColorToken.OnPrimary).radius(Radius.SM))
    state.builder.focusable(retry, 0i32); state.builder.test_id(retry, "error-retry")
    var rs = ui_internal::semantics_empty(); rs.role = Role.Button; rs.label = "Retry"
    state.builder.semantics(retry, rs)
    state.builder.child(card, retry)
}

@retained
public func headless_breakpoint(w : float) : Breakpoint {
    if(w < 600.0f) { return Breakpoint.Sm }
    if(w < 900.0f) { return Breakpoint.Md }
    if(w < 1200.0f) { return Breakpoint.Lg }
    return Breakpoint.Xl
}

@retained
public func headless_theme(options : &Options) : Theme {
    if(options.theme_override is Option.Some) {
        var Some(t) = options.theme_override else return Theme.light()
        return t
    }
    switch(options.theme) {
        ThemeMode.Dark => return Theme.dark()
        ThemeMode.HighContrast => return Theme.high_contrast()
        ThemeMode.Light => return Theme.light()
        ThemeMode.System => return Theme.light()
    }
    return Theme.light()
}

@retained
public func headless_theme_mode(options : &Options) : ThemeMode {
    if(options.theme == ThemeMode.System) { return ThemeMode.Light }
    return options.theme
}

@retained
public func headless_font(options : &Options) : Result<TextContext, UiError> {
    if(options.font_file.size() == 0u) { return Result.Ok<TextContext,UiError>(ui_internal::empty_text_context()) }
    var p = path::PathBuf.from(options.font_file)
    var f = font::Font.from_path(&p.as_path())
    if(f is Result.Err) {
        return Result.Err<TextContext,UiError>(UiError.make(ErrorCode.Font, "pinned font could not be loaded"))
    }
    var Ok(font_value) = f else return Result.Err<TextContext,UiError>(UiError.make(ErrorCode.Font, "font result corrupt"))
    return Result.Ok<TextContext,UiError>(ui_internal::text_context_from_font(font_value))
}

@retained
public func ui_ctx_new(frame : FrameInfo, viewport : Viewport, theme : Theme, textctx : TextContext, stats : FrameStats, focus : Option<Key>, text_scale : float) : UiCtx {
    return UiCtx {
        frame_value : frame, viewport_value : viewport,
        theme_value : theme, textctx_value : textctx,
        stats_value : stats, dirty_value : true, quit_value : false,
        text_scale_value : text_scale,
        focus_value : focus, reported_error_value : Option.None<UiError>(),
        event_kind_value : EventKind.Frame, event_key_value : zeroed<Key>(),
        event_code_value : KeyCode.Unknown, event_repeat_value : false,
        event_shift_value : false, event_text_value : "",
        event_scroll_value : Point.zero(),
        scroll_key_value : zeroed<Key>(),
        scroll_point_value : Point.zero(), has_scroll_value : false
    }
}
@retained
public func ui_ctx_quit(ctx : &UiCtx) : bool { return ctx.quit_value }
@retained
public func ui_ctx_dirty(ctx : &UiCtx) : bool { return ctx.dirty_value }
@retained
public func ui_ctx_theme(ctx : &UiCtx) : Theme { return ctx.theme_value }
@retained
public func ui_ctx_focus(ctx : &UiCtx) : Option<Key> { return ctx.focus_value }
@retained
public func ui_ctx_set_theme(ctx : &mut UiCtx, theme : Theme) { ctx.theme_value = theme }
@retained
public func ui_ctx_clear_dirty(ctx : &mut UiCtx) { ctx.dirty_value = false }
@retained
public func ui_ctx_reported(ctx : &mut UiCtx) : Option<UiError> { return headless_report_take(ctx) }

@retained
public func headless_make_frame(options : &Options, viewport : Viewport, scale : float, frame_no : u64) : FrameInfo {
    var now = 0.0
    if(!options.fixed_time) { now = frame_no as double * 16.0 }
    return FrameInfo {
        window : zeroed<WindowId>(), viewport : viewport,
        scale : scale,
        text_scale : options.font_size_scale,
        theme : headless_theme_mode(options), reduced_motion : options.reduced_motion,
        locale : options.locale, direction : Direction.Ltr, frame : frame_no,
        now_ms : now,
        focused : true, occluded : false
    }
}

@retained
public func headless_zero_stats() : FrameStats {
    return FrameStats {
        frame : 0u64, renderer : Renderer.Cpu, gpu_backend : GpuBackend.Auto,
        build_us : 0.0, layout_us : 0.0, paint_us : 0.0, gpu_us : 0.0,
        present_us : 0.0, event_to_update_us : 0.0, update_to_submit_us : 0.0,
        submit_to_present_us : 0.0, nodes : 0u32, visited : 0u32,
        relaid_out : 0u32, layout_nodes : 0u32, virtual_rebuilds : 0u32,
        virtual_extent_calls : 0u32, virtual_index_builds : 0u32,
        commands : 0u32, glyphs : 0u32, text_measured : 0u32,
        text_cache_hits : 0u32, tiles_painted : 0u32, full_repaints : 0u32,
        paint_bytes : 0u64, scroll_blit_bytes : 0u64, scroll_blit_us : 0.0,
        allocs : 0u32, allocs_cold : 0u32, alloc_bytes_high_water : 0u64,
        settle_frames : 0u32, gpu_upload_bytes : 0u64, gpu_draw_calls : 0u32
    }
}

@retained
public func headless_validate(options : &Options) : Result<Unit, UiError> {
    if(options.width <= 0 || options.height <= 0) {
        return Result.Err<Unit,UiError>(UiError.make(ErrorCode.InvalidArgument, "width and height must be positive"))
    }
    if(options.min_width < 0 || options.min_height < 0) {
        return Result.Err<Unit,UiError>(UiError.make(ErrorCode.InvalidArgument, "min size must not be negative"))
    }
    if(options.scale < 0.0f || options.scale != options.scale || !finite32(options.scale)) {
        return Result.Err<Unit,UiError>(UiError.make(ErrorCode.InvalidArgument, "scale must be finite and non-negative"))
    }
    if(options.font_size_scale <= 0.0f || options.font_size_scale != options.font_size_scale) {
        return Result.Err<Unit,UiError>(UiError.make(ErrorCode.InvalidArgument, "font size scale must be positive"))
    }
    if(options.max_frames > 100000u32) {
        return Result.Err<Unit,UiError>(UiError.make(ErrorCode.InvalidArgument, "max_frames exceeds headless limit"))
    }
    return Result.Ok<Unit,UiError>(Unit{})
}

@retained
public func headless_shot_hash(buf : &ui_internal::ColorBuffer) : u64 {
    var h : u64 = 1469598103934665603u64
    var i : size_t = 0u
    while(i < buf.pixels.size()) {
        h = (h ^ (unsafe(buf.pixels.get_ptr(i)[0]) as u64)) * 1099511628211u64
        i = i + 1u
    }
    return h
}

@retained
public func <A> headless_run_impl(app : &mut A, options : Options, out_path : string_view) : Result<Screenshot, UiError> where A : Application {
    var v = headless_validate(&options)
    if(v is Result.Err) { var Err(e) = v else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "validation result corrupt")); return Result.Err<Screenshot,UiError>(e) }
    var scale = options.scale
    if(scale == 0.0f) { scale = 1.0f }
    var logical_w = options.width as float
    var logical_h = options.height as float
    var physical_w = (logical_w * scale) as u32
    var physical_h = (logical_h * scale) as u32
    if(physical_w == 0u32 || physical_h == 0u32) { return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.InvalidArgument, "physical surface is empty")) }
    var pixels64 = (physical_w as u64) * (physical_h as u64)
    if(options.limits.max_screenshot_pixels != 0u64 && pixels64 > options.limits.max_screenshot_pixels) {
        return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.ResourceExhausted, "screenshot exceeds pixel limit"))
    }
    var capacity = options.limits.max_view_nodes
    if(capacity == 0u32) { capacity = 8192u32 }
    var viewport = Viewport { size : Size.new(logical_w, logical_h), safe_area : Insets.zero(), breakpoint : headless_breakpoint(logical_w) }
    var tc = headless_font(&options)
    if(tc is Result.Err) { var Err(e) = tc else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "font result corrupt")); return Result.Err<Screenshot,UiError>(e) }
    var Ok(textctx) = tc else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "font context corrupt"))
    var layout_ctx = ui_internal::new_layout_context(textctx, logical_w, logical_h)
    var display_cap = options.limits.max_display_commands
    if(display_cap == 0u32) { display_cap = 8192u32 }
    var state = HeadlessState {
        options : options, frame : headless_make_frame(&options, viewport, scale, 0u64), viewport : viewport,
        theme : headless_theme(&options), layout_ctx : layout_ctx,
        builder : ui_internal::new_view_builder(capacity, logical_w, logical_h),
        retained : ui_internal::new_retained_tree(capacity),
        layout : ui_internal::new_layout_state(capacity),
        display : ui_internal::DisplayState.new(display_cap),
        buffer : ui_internal::ColorBuffer.new(physical_w, physical_h),
        stats : headless_zero_stats(), focus_key : Option.None<Key>(),
        hover_key : Option.None<Key>(), pressed_key : Option.None<Key>(),
        capture_key : Option.None<Key>(), last_click_key : Option.None<Key>(),
        last_click_ms : 0.0, last_error : Option.None<UiError>(), error_active : false,
        error_retries : 0u32, focus_before_error : Option.None<Key>(),
        dirty : true, quit : false, frame_no : 0u64,
        frames_run : 0u32, view_calls : 0u32, buffer_ready : false
    }
    var ctx = UiCtx {
        frame_value : state.frame, viewport_value : state.viewport,
        theme_value : state.theme, textctx_value : ui_internal::empty_text_context(),
        stats_value : state.stats, dirty_value : true, quit_value : false,
        text_scale_value : state.frame.text_scale,
        focus_value : state.focus_key, reported_error_value : Option.None<UiError>(),
        event_kind_value : EventKind.Frame, event_key_value : zeroed<Key>(),
        event_code_value : KeyCode.Unknown, event_repeat_value : false,
        event_shift_value : false, event_text_value : "",
        event_scroll_value : Point.zero(),
        scroll_key_value : zeroed<Key>(),
        scroll_point_value : Point.zero(), has_scroll_value : false
    }
    var start_err = headless_fault_take(1u8)
    var start : Result<Unit,UiError>
    if(start_err is Option.Some) {
        var se = headless_option_error(start_err, UiError.make(ErrorCode.Internal, "fault corrupt"))
        start = Result.Err<Unit,UiError>(se)
    } else {
        start = app.on_start(&state.frame, &mut ctx)
    }
    if(start is Result.Err) {
        var Err(e) = start else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "on_start result corrupt"))
        headless_set_error(&mut state, e)
    } else {
        state.dirty = ctx.dirty_value; state.quit = ctx.quit_value
        state.theme = ctx.theme_value; state.frame.theme = ctx.frame_value.theme
    }
    var start_report = headless_report_take(&mut ctx)
    if(start_report.is_some()) { headless_set_error(&mut state, headless_option_error(start_report, UiError.make(ErrorCode.Internal, "reported error corrupt"))) }
    if(headless_fault_take(5u8) is Option.Some) { headless_set_error(&mut state, UiError.make(ErrorCode.Font, "injected font fault")) }
    var max_frames = options.max_frames
    if(max_frames == 0u32) { max_frames = 1u32 }
    while(!state.quit && state.frames_run < max_frames) {
        var bstart = std::chrono::Instant.now()
        state.frame.frame = state.frame_no
        state.frame.viewport = state.viewport
        if(options.fixed_time) { state.frame.now_ms = 0.0 } else { state.frame.now_ms = state.frame_no as double * 16.0 }
        state.builder.frame_data = state.frame
        state.builder.viewport_data = state.viewport
        var card_frame = state.error_active
        if(!card_frame) {
            var view_fault = headless_fault_take(2u8)
            if(view_fault is Option.Some) {
                var ve = headless_option_error(view_fault, UiError.make(ErrorCode.Internal, "view fault corrupt"))
                headless_set_error(&mut state, ve)
                card_frame = true
            }
        }
        if(card_frame) {
            headless_build_error_card(&mut state)
        } else {
            ui_internal::reset_view_builder(&mut state.builder)
            var vr = app.view(&state.frame, &mut state.builder)
            if(vr is Result.Err) {
                var Err(e) = vr else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "view result corrupt"))
                headless_set_error(&mut state, e)
                headless_build_error_card(&mut state)
            } else {
                var Ok(returned_root) = vr else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "view root corrupt"))
                if(!returned_root.is_valid()) {
                    headless_set_error(&mut state, UiError.make(ErrorCode.Layout, "application returned an invalid root"))
                    headless_build_error_card(&mut state)
                } else if(ui_internal::view_diagnostic_count(&state.builder) != 0u32) {
                    var diag = ui_internal::view_last_diagnostic(&state.builder)
                    headless_set_error(&mut state, UiError.make(diag, "view builder rejected a node"))
                    headless_build_error_card(&mut state)
                } else {
                    var be = ui_internal::view_first_error(&state.builder)
                    if(be is Option.Some) {
                        var ve2 = headless_option_error(be, UiError.make(ErrorCode.Internal, "view error corrupt"))
                        headless_set_error(&mut state, ve2)
                        headless_build_error_card(&mut state)
                    } else {
                        state.view_calls = state.view_calls + 1u32
                        if(scale != 1.0f) { state.builder.transform(ui_internal::view_root(&state.builder), Transform2D.scale(scale, scale)) }
                    }
                }
            }
        }
        var rec = ui_internal::reconcile(&mut state.retained, &state.builder)
        if(rec is Result.Err) {
            var Err(e) = rec else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "reconcile result corrupt"))
            headless_set_error(&mut state, e)
            headless_build_error_card(&mut state)
            var rec2 = ui_internal::reconcile(&mut state.retained, &state.builder)
            if(rec2 is Result.Err) { return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "error-card reconcile failed")) }
        }
        var build_us = bstart.elapsed().as_micros() as double
        var lstart = std::chrono::Instant.now()
        var layout_stats = ui_internal::layout_run(&mut state.layout, &state.builder, &state.layout_ctx, &state.theme)
        var layout_us = lstart.elapsed().as_micros() as double
        var layout_fault = headless_fault_take(4u8)
        if(layout_fault is Option.Some || ui_internal::layout_warning_count(&state.layout) != 0u32) {
            var le = UiError.make(ErrorCode.Layout, "layout produced warnings")
            if(layout_fault.is_some()) { le = headless_option_error(layout_fault, le) }
            headless_set_error(&mut state, le)
            headless_build_error_card(&mut state)
            layout_stats = ui_internal::layout_run(&mut state.layout, &state.builder, &state.layout_ctx, &state.theme)
        }
        state.display.focus_key = state.focus_key
        state.display.hover_key = state.hover_key
        var pstart = std::chrono::Instant.now()
        var _commands = ui_internal::display_build(&mut state.display, &state.builder, &state.layout, &state.theme, &state.layout_ctx.text, Size.new(physical_w as float, physical_h as float))
        var paint_err = headless_paint_canvases(app, &mut state, scale)
        if(paint_err.is_some()) {
            headless_set_error(&mut state, headless_option_error(paint_err, UiError.make(ErrorCode.Internal, "paint fault corrupt")))
            headless_build_error_card(&mut state)
            var prec = ui_internal::reconcile(&mut state.retained, &state.builder)
            if(prec is Result.Err) { return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "error-card reconcile failed")) }
            layout_stats = ui_internal::layout_run(&mut state.layout, &state.builder, &state.layout_ctx, &state.theme)
            _commands = ui_internal::display_build(&mut state.display, &state.builder, &state.layout, &state.theme, &state.layout_ctx.text, Size.new(physical_w as float, physical_h as float))
        }
        var paint_us = pstart.elapsed().as_micros() as double
        if(state.focus_key.is_some()) {
            var fk = headless_option_key(state.focus_key, zeroed<Key>())
            if(ui_internal::hit_focusable_by_key(&state.display, fk).is_none()) {
                state.focus_key = Option.None<Key>()
                state.dirty = true
            }
        }
        var raster = ui_internal::Rasterizer.new(&raw mut state.buffer)
        var _rstats = raster.render(&state.display.list)
        state.buffer_ready = true
        state.stats = FrameStats {
            frame : state.frame_no, renderer : Renderer.Cpu, gpu_backend : GpuBackend.Auto,
            build_us : build_us, layout_us : layout_us, paint_us : paint_us,
            gpu_us : 0.0, present_us : 0.0, event_to_update_us : 0.0,
            update_to_submit_us : 0.0, submit_to_present_us : 0.0,
            nodes : ui_internal::view_node_count(&state.builder), visited : ui_internal::view_node_count(&state.builder),
            relaid_out : layout_stats.visited, layout_nodes : ui_internal::layout_node_count(&state.layout),
            virtual_rebuilds : 0u32, virtual_extent_calls : 0u32,
            virtual_index_builds : 0u32, commands : ui_internal::display_command_count(&state.display),
            glyphs : 0u32, text_measured : 0u32, text_cache_hits : 0u32,
            tiles_painted : 0u32, full_repaints : 1u32,
            paint_bytes : (physical_w as u64) * (physical_h as u64) * 4u64,
            scroll_blit_bytes : 0u64, scroll_blit_us : 0.0, allocs : 0u32,
            allocs_cold : 0u32, alloc_bytes_high_water : 0u64,
            settle_frames : state.frames_run + 1u32, gpu_upload_bytes : 0u64,
            gpu_draw_calls : 0u32
        }
        state.frames_run = state.frames_run + 1u32
        state.frame_no = state.frame_no + 1u64
        state.dirty = false
        ctx.frame_value = state.frame
        ctx.stats_value = state.stats
        ctx.text_scale_value = state.frame.text_scale
        ctx.focus_value = state.focus_key
        if(headless_queue_size() > 0u32) {
            var er = headless_dispatch_events(app, &mut state, &mut ctx)
            if(er is Result.Err) {
                var Err(e) = er else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "event result corrupt"))
                headless_set_error(&mut state, e)
                headless_queue_clear()
            }
        } else {
            var tick = UiEvent {
                window : zeroed<WindowId>(), key : zeroed<Key>(), kind : EventKind.Frame,
                payload : Payload.None(), modifiers : Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false },
                local_x : 0.0f, local_y : 0.0f, timestamp_ms : state.frame.now_ms,
                frame : state.frame_no - 1u64, pointer_button : 0u8,
                scroll_dx : 0.0f, scroll_dy : 0.0f, key_code : KeyCode.Unknown,
                key_repeat : false, text_value : ""
            }
            var ur = app.update(&tick, &mut ctx)
            if(ur is Result.Err) {
                var Err(e) = ur else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "update result corrupt"))
                headless_set_error(&mut state, e)
            } else {
                state.dirty = ctx.dirty_value
                state.quit = ctx.quit_value
                state.theme = ctx.theme_value
                state.frame.theme = ctx.frame_value.theme
            }
        }
        state.dirty = ctx.dirty_value || state.error_active
        state.focus_key = ctx.focus_value
        if(state.error_active) { state.focus_key = Option.Some<Key>(headless_error_retry_key()) }
        if(state.quit) { break }
        // Headless has no event source or animation owner yet. One stable
        // frame is the settle point; max_frames > 1 repeats the same stable
        // frame for deterministic timing tests.
    }
    var _close = app.on_close(&state.frame, &mut ctx)
    if(!state.buffer_ready) { return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "headless loop produced no frame")) }
    if(out_path.size() == 0u) { return Result.Ok<Screenshot,UiError>(Screenshot { path : "", width : physical_w, height : physical_h, hash : 0u64 }) }
    var outp = path::PathBuf.from(out_path)
    var saved = ui_internal::raster_save_png(&state.buffer, &outp.as_path())
    if(saved is Result.Err) { return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Io, "headless PNG write failed")) }
    return Result.Ok<Screenshot,UiError>(Screenshot { path : out_path, width : physical_w, height : physical_h, hash : headless_shot_hash(&state.buffer) })
}

@retained
public func <A> run_headless(app : &mut A, options : Options) : Result<Unit, UiError> where A : Application {
    var shot = headless_run_impl(app, options, "")
    if(shot is Result.Err) { var Err(e) = shot else return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Internal, "headless result corrupt")); return Result.Err<Unit,UiError>(e) }
    return Result.Ok<Unit,UiError>(Unit{})
}

// P1.10 screenshot path helper. The shot app and tests use this; P6.1's
// UiHarness is the public long-term boundary and may wrap it.
@retained
public func <A> run_headless_shot(app : &mut A, options : Options, out_path : string_view) : Result<Screenshot, UiError> where A : Application {
    return headless_run_impl(app, options, out_path)
}

