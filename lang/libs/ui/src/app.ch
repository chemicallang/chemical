// P1.10 headless app loop. No window, input, focus, IME, or multi-window.
// The public surface implements the API.md app/options/context subset needed
// for headless rendering; P1.11+ extends UiCtx.
using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;

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

public struct UiEvent {
    var frame : u64
    var timestamp_ms : double
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

    public func frame_info(&self) : &FrameInfo { return &self.frame_value }
    public func viewport(&self) : Viewport { return self.viewport_value }
    public func scale(&self) : float { return self.frame_value.scale }
    public func text_scale(&self) : float { return self.text_scale_value }
    public func text_context(&self) : &TextContext { return &self.textctx_value }
    public func theme(&self) : &Theme { return &self.theme_value }
    public func theme_mode(&self) : ThemeMode { return self.frame_value.theme }
    public func now_ms(&self) : double { return self.frame_value.now_ms }
    public func frame(&self) : u64 { return self.frame_value.frame }
    public func stats(&self) : FrameStats { return self.stats_value }
    public func request_view(&mut self) { self.dirty_value = true }
    public func invalidate(&mut self, key : Key) { self.dirty_value = true }
    public func request_paint(&mut self) {}
    public func request_animation_frame(&mut self) {}
    public func request_frame(&mut self) {}
    public func request_quit(&mut self) { self.quit_value = true }
    public func set_theme(&mut self, theme : Theme) { self.theme_value = theme; self.dirty_value = true }
    public func set_theme_mode(&mut self, mode : ThemeMode) { self.frame_value.theme = mode; self.dirty_value = true }
    public func set_text_scale(&mut self, scale : float) {
        if(scale > 0.0f && scale == scale && finite32(scale)) { self.text_scale_value = scale; self.dirty_value = true }
    }
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
        stats : headless_zero_stats(), dirty : true, quit : false, frame_no : 0u64,
        frames_run : 0u32, view_calls : 0u32, buffer_ready : false
    }
    var ctx = UiCtx {
        frame_value : state.frame, viewport_value : state.viewport,
        theme_value : state.theme, textctx_value : ui_internal::empty_text_context(),
        stats_value : state.stats, dirty_value : true, quit_value : false,
        text_scale_value : state.frame.text_scale
    }
    var start = app.on_start(&state.frame, &mut ctx)
    if(start is Result.Err) { var Err(e) = start else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "on_start result corrupt")); return Result.Err<Screenshot,UiError>(e) }
    state.dirty = ctx.dirty_value; state.quit = ctx.quit_value
    state.theme = ctx.theme_value; state.frame.theme = ctx.frame_value.theme
    var max_frames = options.max_frames
    if(max_frames == 0u32) { max_frames = 1u32 }
    while(!state.quit && state.frames_run < max_frames) {
        var bstart = std::chrono::Instant.now()
        state.frame.frame = state.frame_no
        state.frame.viewport = state.viewport
        if(options.fixed_time) { state.frame.now_ms = 0.0 } else { state.frame.now_ms = state.frame_no as double * 16.0 }
        state.builder.frame_data = state.frame
        state.builder.viewport_data = state.viewport
        ui_internal::reset_view_builder(&mut state.builder)
        var vr = app.view(&state.frame, &mut state.builder)
        if(vr is Result.Err) { var Err(e) = vr else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "view result corrupt")); return Result.Err<Screenshot,UiError>(e) }
        var Ok(returned_root) = vr else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "view root corrupt"))
        if(!returned_root.is_valid()) { return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Layout, "application returned an invalid root")) }
        state.view_calls = state.view_calls + 1u32
        if(ui_internal::view_diagnostic_count(&state.builder) != 0u32) {
            return Result.Err<Screenshot,UiError>(UiError.make(ui_internal::view_last_diagnostic(&state.builder), "view builder rejected a node"))
        }
        if(scale != 1.0f) {
            state.builder.transform(ui_internal::view_root(&state.builder), Transform2D.scale(scale, scale))
        }
        var rec = ui_internal::reconcile(&mut state.retained, &state.builder)
        if(rec is Result.Err) { var Err(e) = rec else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "reconcile result corrupt")); return Result.Err<Screenshot,UiError>(e) }
        var build_us = bstart.elapsed().as_micros() as double
        var lstart = std::chrono::Instant.now()
        var layout_stats = ui_internal::layout_run(&mut state.layout, &state.builder, &state.layout_ctx, &state.theme)
        var layout_us = lstart.elapsed().as_micros() as double
        if(ui_internal::layout_warning_count(&state.layout) != 0u32) {
            return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Layout, "layout produced warnings"))
        }
        var pstart = std::chrono::Instant.now()
        var _commands = ui_internal::display_build(&mut state.display, &state.builder, &state.layout, &state.theme, &state.layout_ctx.text, Size.new(physical_w as float, physical_h as float))
        var paint_us = pstart.elapsed().as_micros() as double
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
        var tick = UiEvent { frame : state.frame_no - 1u64, timestamp_ms : state.frame.now_ms }
        var ur = app.update(&tick, &mut ctx)
        if(ur is Result.Err) { var Err(e) = ur else return Result.Err<Screenshot,UiError>(UiError.make(ErrorCode.Internal, "update result corrupt")); return Result.Err<Screenshot,UiError>(e) }
        state.dirty = ctx.dirty_value
        state.quit = ctx.quit_value
        state.theme = ctx.theme_value
        state.frame.theme = ctx.frame_value.theme
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

