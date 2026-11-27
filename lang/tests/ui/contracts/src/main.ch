// P0.2 frozen contract module. This file defines every public declaration in
// API.md with exact signatures. Bodies are `@extern` or empty; this module never
// runs UI code. Keeping it compiling on TCC and LLVM is the contract gate.
//
// Namespace ownership follows API.md §0: all §1–§9/§11 declarations are in
// `ui` unless a separate namespace is named.

using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;

public namespace ui {

// ── 1. App model ───────────────────────────────────────────────────────────
@direct_init
public struct FrameInfo {
    var window : WindowId
    var viewport : Viewport
    var scale : float
    var text_scale : float
    var theme : ThemeMode
    var reduced_motion : bool
    var locale : string_view
    var direction : Direction
    var frame : u64
    var now_ms : double
    var focused : bool
    var occluded : bool
}

@direct_init
public struct Viewport {
    var size : Size
    var safe_area : Insets
    var breakpoint : Breakpoint
}

public interface Application {
    func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit, UiError>
    func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId, UiError>
    func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit, UiError>
    func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit, UiError>
    func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction
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
    @make @extern func make() : Options
}

@direct_init public struct Point { var x : float; var y : float }
@direct_init public struct Rect { var x : float; var y : float; var w : float; var h : float }
@direct_init public struct Transform2D { var m : [6]float }
@direct_init public struct Shadow { var x : float; var y : float; var blur : float; var spread : float; var color : Color }
@direct_init public struct TextSpan {
    var text : string_view; var role : TextRole; var color : ColorToken
    var weight : FontWeight; var italic : bool; var family : FontFamily
    var features : TextFeatures; var link : Option<string_view>; var decoration : u8
}
@direct_init public struct GridTracks { var columns : &[GridTrack]; var rows : &[GridTrack]; var gap : Space }
public variant GridTrack { Px(value : float) Fr(value : float) Auto() }
@direct_init public struct Duration { var ms : float }
public enum Easing { Linear, EaseIn, EaseOut, EaseInOut, Spring }
public enum MotionToken { Fast, Base, Slow, Spring }
public enum AnimProperty { Opacity, TranslateX, TranslateY, Scale, Rotate, Background, Width, Height }
@direct_init public struct AnimationSpec {
    var duration : Duration; var easing : Easing; var delay : Duration
    var repeat : u32; var alternate : bool
}
public enum Cursor { Arrow, IBeam, Hand, Wait, Crosshair, Move, ResizeNS, ResizeEW, ResizeNESW, ResizeNWSE, NotAllowed }
public enum ImageFit { Contain, Cover, Fill, None }
public enum FontFamily { Ui, Mono, System }
public struct FontSpec {
    var family : FontFamily; var family_name : string_view
    var file : string_view; var fallback : FontFamily
}
@direct_init public struct FontFallback { var script : u32; var families : &[string_view] }
@direct_init public struct FontStack {
    var ui : &[string_view]; var mono : &[string_view]
    var fallback : &[FontFallback]
    @make func system() : FontStack {
        return zeroed<FontStack>()
    }
}
public struct IconId { }
public struct FontHandle { }
public struct ImageHandle { }
@direct_init public struct FontCacheEntry { var family : string_view; var path : string_view; var sha256 : string_view; var bytes : u64; var license : string_view; var source_url : string_view; var version : string_view }
public variant FontError {
    NotFound(family : string_view)
    InvalidFormat()
    LicenseRequired()
    DownloadFailed()
    CacheFailed()
    OutOfMemory()
}
public variant ImageSource { Path(path : string_view) Bytes(bytes : &[u8]) Handle(h : ImageHandle) Placeholder() }

public enum Renderer { Auto, Cpu, Gpu }
public enum GpuBackend { Auto, Vulkan, Metal, D3D12, WebGpu, OpenGl }
public enum GpuPreference { Auto, Discrete, Integrated, Software }
@direct_init
public struct GpuInfo {
    var available : bool
    var backend : GpuBackend
    var device : string_view
    var driver : string_view
    var dedicated : bool
    var max_texture : u32
    var supports_compute : bool
    var software : bool
}
public struct WindowId { }
public enum ThemeMode { Light, Dark, HighContrast, System }
public enum Density { Comfortable, Compact }
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

public variant ImageState { Missing() Loading() Ready(value : ImageHandle) Failed(err : UiError) }
public struct FontRequest { }

public enum Breakpoint { Sm, Md, Lg, Xl }
@direct_init public struct MenuItem {
    var id : CommandId; var label : string_view; var shortcut : Option<Shortcut>
    var enabled : bool; var checked : bool; var separator : bool
}
@direct_init public struct Menu { var title : string_view; var items : &[MenuItem] }
@direct_init public struct FileDialogOptions {
    var title : string_view; var filters : &[string_view]
    var multiple : bool; var directory : bool; var save : bool
}
@direct_init public struct Notification {
    var title : string_view; var body : string_view; var icon : Option<ImageHandle>
}
@direct_init public struct WindowSpec {
    var title : string_view; var size : Size; var min_size : Size; var max_size : Size
    var position : Option<Point>; var resizable : bool; var decorated : bool
    var always_on_top : bool; var transparent : bool; var start_visible : bool
}
@direct_init public struct CanvasPaint {
    var node : NodeId; var key : Key; var id : CanvasId
    var bounds : Rect; var scale : float
}
public struct TimerId { }
public struct CommandId { private var value : u32 }
@direct_init public struct Shortcut { var code : KeyCode; var mods : Modifiers }
@direct_init public struct SourceLocation { var file : string_view; var line : u32; var col : u32 }
public enum RetryHint { Never, Immediate, Backoff }
@direct_init public struct ErrorDetail {
    var code : u32; var message : &'static str; var context : &'static str
}
@direct_init public struct UserEvent {
    var tag : u64; var a : i64; var b : i64; var text : string_view
}
public struct EventSender {
    @extern public func post(&self, ev : UserEvent) : Result<Unit, UiError>
    @extern public func wake(&self)
}
@direct_init public struct Limits {
    var max_view_nodes : u32; var max_retained_nodes : u32
    var max_text_bytes : u64; var max_image_pixels : u64; var max_image_bytes : u64
    var max_font_bytes : u64; var max_atlas_bytes : u64
    var max_layer_bytes : u64; var max_gpu_bytes : u64
    var max_display_commands : u32; var max_protocol_bytes : u32
    var max_capture_frames : u32; var max_screenshot_pixels : u64
    @make func make() : Limits {
        return zeroed<Limits>()
    }
}
@direct_init public struct Range { var start : u32; var end : u32 }
public variant ImeTestEvent {
    Preedit(text : string_view, caret : u32, anchor : u32, replace : Range)
    Commit(text : string_view, replace : Range)
    Cancel()
}
@direct_init public struct ImeComposition {
    var preedit : string_view; var caret : u32; var anchor : u32
    var replace : Range; var candidate : Option<Rect>
}
@direct_init public struct TextEditState {
    var text : string_view; var anchor : u32; var caret : u32
    var composition : Option<ImeComposition>; var secure : bool
}
public variant PathCommand {
    MoveTo(p : Point) LineTo(p : Point) QuadTo(c : Point, p : Point)
    CubicTo(c1 : Point, c2 : Point, p : Point) Close()
}
@direct_init public struct Path { var commands : vector<PathCommand>; var fill_rule : u8 }
@direct_init public struct GradientStop { var offset : float; var color : Color }
public variant Gradient { Linear(start : Point, end : Point, stops : vector<GradientStop>) Radial(center : Point, radius : float, stops : vector<GradientStop>) }
public struct ScopeGuard { }
public struct ClipGuard { }
public struct LayerGuard { }
public struct TransformGuard { }
@direct_init public struct Screenshot {
    var path : string_view; var width : u32; var height : u32; var hash : u64
}
@direct_init public struct GoldenOptions {
    var max_diff_pixels : u32
    var yiq : float
    var masks : &[Rect]
    @make func make() : GoldenOptions {
        return zeroed<GoldenOptions>()
    }
}
@direct_init public struct GoldenResult {
    var passed : bool; var diff_path : string_view; var new_path : string_view
}
@direct_init public struct Query {
    var role : Option<Role>; var name : Option<string_view>
    var key : Option<Key>; var test_id : Option<string_view>
    var text : Option<string_view>; var component : Option<string_view>
    var state : Option<WidgetState>; var within : Option<NodeId>
    var all : bool; var limit : u32
}
public enum SelectionMode { None, Single, Multiple }
@direct_init public struct TableColumn {
    var id : string_view; var title : string_view; var width : Dim
    var align : TextAlign; var sortable : bool
}
public variant SortSpec { Ascending(column : string_view) Descending(column : string_view) }
public interface ItemSource {
    func count(&self) : size_t
    func key(&self, index : size_t) : Key
    func item(&self, b : &mut ViewBuilder, index : size_t) : NodeId
    func extent(&self, index : size_t) : float
}
public interface TableSource {
    func count(&self) : size_t
    func row_key(&self, index : size_t) : Key
    func row_extent(&self, index : size_t) : float
    func cell(&self, b : &mut ViewBuilder, row : size_t, column : string_view) : NodeId
    func sort(&mut self, spec : SortSpec)
    func filter(&mut self, query : string_view)
}
public struct Component { }
public struct ComponentSpec { }
public struct ComponentMeta { }
public struct PageSpec { }
public struct AuditReport { }

@direct_init
public struct SurfaceDesc {
    var width : u32
    var height : u32
    var stride : u32
    var format : SurfaceFormat
}
public enum SurfaceFormat { Bgra8Premul, Rgba8Premul, Rgba8SrgbPremul }
public struct Surface<'a> {
    var desc : SurfaceDesc
    var bytes : usize
    var data : *mut u8
}
public interface Presenter {
    func <'a> acquire(&'a mut self, desc : SurfaceDesc) : Result<Surface<'a>, UiError>
    func present(&mut self, damage : &[Rect]) : Result<Unit, UiError>
    func resize(&mut self, width : u32, height : u32, scale : float) : Result<Unit, UiError>
    func request_frame(&mut self)
    func set_cursor(&mut self, c : Cursor)
    func set_ime_rect(&mut self, r : Rect)
}
@direct_init
public struct HostCaps {
    var partial_present : bool; var vsync : bool; var multi_window : bool
    var ime : bool; var clipboard_images : bool; var drop_files : bool
    var native_menus : bool; var notifications : bool; var a11y : bool
}

// ── 2. Identity ────────────────────────────────────────────────────────────
public struct NodeId {
    private var index : u32
    private var generation : u32
}
public struct Key {
    private var hash : u64
    private var tag_value : u64
    private var id_value : u64
    @extern public func equals(&self, label : string_view) : bool
    @extern public func equals_key(&self, other : Key) : bool
    @extern public func id(&self) : u64
    @extern public func tag(&self) : u64
}
@extern public func key(label : string_view) : Key
@extern public func key_of(label : string_view, id : u64) : Key
@extern public func key_pair(label : string_view, a : u64, b : u64) : Key
public struct CanvasId { }

// ── 3. View construction ───────────────────────────────────────────────────
public struct ViewBuilder {
    @extern public func frame(&self) : &FrameInfo
    @extern public func viewport(&self) : Viewport
    @extern public func scroll_offset(&self, key : Key) : Point
    @extern public func row(&mut self, key : Key, style : Style) : NodeId
    @extern public func column(&mut self, key : Key, style : Style) : NodeId
    @extern public func stack(&mut self, key : Key, style : Style) : NodeId
    @extern public func wrap(&mut self, key : Key, style : Style) : NodeId
    @extern public func grid(&mut self, key : Key, style : Style, tracks : GridTracks) : NodeId
    @extern public func scroll(&mut self, key : Key, style : Style, axis : Axis, anchor : ScrollAnchor = ScrollAnchor.Start()) : NodeId
    @extern public func canvas(&mut self, key : Key, style : Style, canvas : CanvasId) : NodeId
    @extern public func fragment(&mut self, key : Key) : NodeId
    @extern public func scope(&mut self, key : Key) : ScopeGuard
    @extern public func scoped_key(&self, parent : Key, label : string_view) : Key
    @extern public func overlay(&mut self, key : Key, layer : OverlayLayer, anchor : Option<NodeId>) : NodeId
    @extern public func component(&mut self, key : Key, name : string_view) : Component
    @extern public func slot(&mut self, c : Component, name : string_view) : NodeId
    @extern public func text(&mut self, key : Key, value : string_view, style : Style) : NodeId
    @extern public func rich_text(&mut self, key : Key, spans : &[TextSpan], style : Style) : NodeId
    @extern public func image(&mut self, key : Key, src : ImageSource, style : Style) : NodeId
    @extern public func icon(&mut self, key : Key, icon : IconId, style : Style) : NodeId
    @extern public func <S : ItemSource> virtual_list(&mut self, key : Key, style : Style, axis : Axis, source : &S) : NodeId
    @extern public func child(&mut self, parent : NodeId, child : NodeId)
    @extern public func children(&mut self, parent : NodeId, kids : &[NodeId])
    @extern public func insert_child(&mut self, parent : NodeId, index : u32, child : NodeId)
    @extern public func style(&mut self, node : NodeId, style : Style)
    @extern public func state_style(&mut self, node : NodeId, state : WidgetState, style : Style)
    @extern public func command(&mut self, node : NodeId, cmd : CommandId)
    @extern public func semantics(&mut self, node : NodeId, sem : Semantics)
    @extern public func test_id(&mut self, node : NodeId, id : string_view)
    @extern public func focusable(&mut self, node : NodeId, tab_index : i32)
    @extern public func grid_cell(&mut self, node : NodeId, col : u16, row : u16, col_span : u16, row_span : u16)
    @extern public func animate(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing)
    @extern public func animate_exit(&mut self, node : NodeId, prop : AnimProperty, to : float, duration : Duration, easing : Easing)
    @extern public func animate_spec(&mut self, node : NodeId, prop : AnimProperty, to : float, spec : AnimationSpec)
    @extern public func animate_exit_spec(&mut self, node : NodeId, prop : AnimProperty, to : float, spec : AnimationSpec)
    @extern public func transform(&mut self, node : NodeId, t : Transform2D)
    @extern public func error(&mut self, node : NodeId, err : UiError)
}

// ── 4. Style and tokens ────────────────────────────────────────────────────
public enum ColorToken {
    Transparent,
    Surface, SurfaceRaised, SurfaceSunken, SurfaceOverlay,
    OnSurface, OnSurfaceMuted, OnSurfaceFaint,
    Primary, OnPrimary, PrimaryHover, PrimaryPressed,
    Secondary, OnSecondary,
    Danger, OnDanger, Success, OnSuccess, Warning, OnWarning,
    Border, BorderStrong, BorderFocus,
    FocusRing, Selection, Scrim,
    Chart1, Chart2, Chart3, Chart4, Chart5, Chart6
}
public enum Space { None, XXS, XS, SM, MD, LG, XL, XXL, XXXL }
public enum Radius { None, XS, SM, MD, LG, XL, Full }
public enum Elevation { None, Low, Medium, High }
public enum TextRole { Display, H1, H2, H3, H4, Body, BodyStrong, Caption, Code, Label }
public enum FontWeight { Regular, Medium, Semibold, Bold }
public enum Overflow { Visible, Clip, Scroll }
public enum Align { Start, Center, End, Stretch, Baseline }
public enum Justify { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly }
public enum Axis { Horizontal, Vertical, Both }
@direct_init public struct Color { var r : u8; var g : u8; var b : u8; var a : u8 }
public variant Dim { Auto() Px(value : float) Percent(value : float) Fill() }
@direct_init public struct Size { var width : float; var height : float }
public enum Direction { Auto, Ltr, Rtl }
public enum TextAlign { Start, Center, End, Justify }
public enum WrapMode { NoWrap, Wrap, WrapReverse }
public enum TextOverflow { Clip, Ellipsis }
public variant Position { Relative() Absolute(top : Dim, right : Dim, bottom : Dim, left : Dim) }
public enum WidgetState { Hover, Focus, Pressed, Disabled, Selected, Checked, Invalid }
public enum OverlayLayer { Tooltip, Popover, Menu, Dialog, Toast }
public variant ScrollAnchor { Start() End() Center() Node(key : Key) }
@direct_init public struct Insets { var left : float; var top : float; var right : float; var bottom : float }
@direct_init public struct Border { var width : float; var color : ColorToken }
public struct Style {
    @extern public func new() : Style
    @extern public func padding(&self, p : Insets) : Style
    @extern public func padding_all(&self, s : Space) : Style
    @extern public func gap(&self, s : Space) : Style
    @extern public func width(&self, s : Dim) : Style
    @extern public func height(&self, s : Dim) : Style
    @extern public func min_width(&self, px : float) : Style
    @extern public func max_width(&self, px : float) : Style
    @extern public func grow(&self, f : float) : Style
    @extern public func shrink(&self, f : float) : Style
    @extern public func basis(&self, s : Dim) : Style
    @extern public func align_items(&self, a : Align) : Style
    @extern public func justify(&self, j : Justify) : Style
    @extern public func align_self(&self, a : Align) : Style
    @extern public func background(&self, c : ColorToken) : Style
    @extern public func foreground(&self, c : ColorToken) : Style
    @extern public func border(&self, b : Border) : Style
    @extern public func radius(&self, r : Radius) : Style
    @extern public func shadow(&self, e : Elevation) : Style
    @extern public func opacity(&self, f : float) : Style
    @extern public func clip(&self, on : bool) : Style
    @extern public func overflow(&self, o : Overflow) : Style
    @extern public func text(&self, role : TextRole) : Style
    @extern public func weight(&self, w : FontWeight) : Style
    @extern public func cursor(&self, c : Cursor) : Style
    @extern public func z(&self, z : i32) : Style
    @extern public func aspect(&self, ratio : float) : Style
    @extern public func position(&self, p : Position) : Style
    @extern public func inset(&self, i : Insets) : Style
    @extern public func grid_column(&self, col : u16, span : u16) : Style
    @extern public func grid_row(&self, row : u16, span : u16) : Style
    @extern public func direction(&self, d : Direction) : Style
    @extern public func text_style(&self, t : TextStyle) : Style
    @extern public func text_align(&self, a : TextAlign) : Style
    @extern public func wrap(&self, w : WrapMode) : Style
    @extern public func max_lines(&self, n : u16) : Style
    @extern public func text_overflow(&self, o : TextOverflow) : Style
    @extern public func tab_size(&self, spaces : u8) : Style
    @extern public func merge(&self, overlay : Style) : Style
}
public struct Theme {
    @extern public func light() : Theme
    @extern public func dark() : Theme
    @extern public func high_contrast() : Theme
    @extern public func color(&self, t : ColorToken) : Color
    @extern public func space(&self, s : Space) : float
    @extern public func radius(&self, r : Radius) : float
    @extern public func text(&self, r : TextRole) : TextStyle
    @extern public func elevation(&self, e : Elevation) : Shadow
    @extern public func density(&self) : Density
    @extern public func text_scale(&self) : float
    @extern public func motion(&self, m : MotionToken) : Duration
    @extern public func set_color(&mut self, t : ColorToken, c : Color)
    @extern public func set_space(&mut self, s : Space, px : float)
    @extern public func set_radius(&mut self, r : Radius, px : float)
    @extern public func set_text_style(&mut self, role : TextRole, t : TextStyle)
    @extern public func set_density(&mut self, d : Density)
    @extern public func disabled_opacity(&self) : float
    @extern public func set_text_scale(&mut self, scale : float)
}

// ── 5. Events and context ─────────────────────────────────────────────────
public enum EventKind {
    Click, DoubleClick, ContextMenu,
    Press, Release, PointerMove, PointerEnter, PointerLeave,
    Scroll, KeyDown, KeyUp, TextInput, ImePreedit, ImeCommit,
    Focus, Blur, Change, Submit, Select, SelectionChanged, TextChanged, Open, Close,
    Drop, DragBegin, DragOver, DragLeave, DragDrop,
    Resize, DpiChange, Timer, Frame, Command, Custom, ImageReady, ImageFailed, FontReady,
    FileChosen, OpenFile, SecondInstance
}
@direct_init public struct Modifiers {
    var shift : bool; var ctrl : bool; var alt : bool; var meta : bool; var caps : bool
}
@direct_init public struct PointerPayload {
    var button : u8; var press_count : u8; var pointer_kind : u8
    var pressure : float; var tilt_x : float; var tilt_y : float
}
@direct_init public struct ScrollPayload {
    var dx : float; var dy : float; var phase : u8
}
@direct_init public struct KeyPayload {
    var code : KeyCode; var repeat : bool; var text : Option<string_view>
}
@direct_init public struct TextPayload { var text : string_view }
@direct_init public struct ImePayload {
    var preedit : string_view; var commit : string_view
    var caret : u32; var anchor : u32
    var replace : Range
    var candidate : Option<Rect>
}
@direct_init public struct SizePayload { var width : float; var height : float; var scale : float }
@direct_init public struct ScalePayload { var scale : float }
@direct_init public struct FilesPayload { var paths : &[string_view]; var text : Option<string_view> }
@direct_init public struct DragPayload {
    var payload : UserEvent; var x : float; var y : float
    var target : Option<Key>; var accepted : bool
}
@direct_init public struct TimerPayload { var id : TimerId }
@direct_init public struct FramePayload { var now_ms : double; var dt_ms : double }
@direct_init public struct ValuePayload {
    var kind : u8; var text : string_view; var number : double
    var range : Range
    var selected : bool; var checked : bool
}
@direct_init public struct FocusPayload { var focused : bool }
@direct_init public struct IndexPayload { var index : u32; var selected : bool }
@direct_init public struct CommandPayload { var id : CommandId }
@direct_init public struct ImagePayload { var handle : ImageHandle }
@direct_init public struct FontPayload { var handle : FontHandle }
@direct_init public struct TextChangePayload { var range : Range; var inserted : string_view }
@direct_init public struct SelectionPayload { var range : Range; var selected : bool }
@direct_init public struct ErrorPayload { var err : UiError }
@direct_init public struct UserPayload {
    var tag : u64; var a : i64; var b : i64; var text : string_view
}
public variant Payload<'a> {
    None()
    Pointer(value : PointerPayload)
    Scroll(value : ScrollPayload)
    Key(value : KeyPayload)
    Text(value : TextPayload)
    Ime(value : ImePayload)
    Size(value : SizePayload)
    Scale(value : ScalePayload)
    Files(value : FilesPayload)
    Drag(value : DragPayload)
    Timer(value : TimerPayload)
    Frame(value : FramePayload)
    Value(value : ValuePayload)
    Selection(value : SelectionPayload)
    TextChange(value : TextChangePayload)
    Focus(value : FocusPayload)
    Index(value : IndexPayload)
    Command(value : CommandPayload)
    Image(value : ImagePayload)
    Font(value : FontPayload)
    Error(value : ErrorPayload)
    User(value : UserPayload)
}
@direct_init
public struct UiEvent<'a> {
    var window : WindowId
    var key : Key
    var kind : EventKind
    var payload : Payload<'a>
    var modifiers : Modifiers
    var local_x : float
    var local_y : float
    var timestamp_ms : double
    var frame : u64
}
public struct UiCtx {
    @extern public func frame_info(&self) : &FrameInfo
    @extern public func viewport(&self) : Viewport
    @extern public func scale(&self) : float
    @extern public func text_scale(&self) : float
    @extern public func set_text_scale(&mut self, scale : float)
    @extern public func request_view(&mut self)
    @extern public func invalidate(&mut self, key : Key)
    @extern public func request_paint(&mut self)
    @extern public func request_quit(&mut self)
    @extern public func request_frame(&mut self)
    @extern public func request_animation_frame(&mut self)
    @extern public func set_timeout(&mut self, ms : u32) : TimerId
    @extern public func cancel_timeout(&mut self, id : TimerId)
    @extern public func text(&self, key : Key) : &str
    @extern public func take_text(&mut self, key : Key) : string
    @extern public func text_context(&self) : &TextContext
    @extern public func text_state(&self, key : Key) : Option<&TextEditState>
    @extern public func set_text(&mut self, key : Key, text : string_view) : Result<Unit, UiError>
    @extern public func set_selection(&mut self, key : Key, anchor : u32, caret : u32)
    @extern public func select_all(&mut self, key : Key)
    @extern public func ime_rect(&mut self, key : Key, rect : Rect)
    @extern public func is_focused(&self, key : Key) : bool
    @extern public func focus(&mut self, key : Key)
    @extern public func scroll_offset(&self, key : Key) : Point
    @extern public func set_scroll(&mut self, key : Key, offset : Point)
    @extern public func scroll_to(&mut self, key : Key)
    @extern public func clipboard(&self) : string_view
    @extern public func set_clipboard(&mut self, text : string_view)
    @extern public func begin_drag(&mut self, source : Key, payload : UserEvent)
    @extern public func accept_drop(&mut self, target : Key, accepted : bool)
    @extern public func open_url(&mut self, url : string_view) : Result<Unit, UiError>
    @extern public func theme_mode(&self) : ThemeMode
    @extern public func set_theme_mode(&mut self, mode : ThemeMode)
    @extern public func set_theme(&mut self, theme : Theme)
    @extern public func theme(&self) : &Theme
    @extern public func density(&self) : Density
    @extern public func set_density(&mut self, d : Density)
    @extern public func disabled_opacity(&self) : float
    @extern public func now_ms(&self) : double
    @extern public func frame(&self) : u64
    @extern public func stats(&self) : FrameStats
    @extern public func sender(&self) : EventSender
    @extern public func window(&self) : WindowId
    @extern public func open_window(&mut self, spec : WindowSpec) : Result<WindowId, UiError>
    @extern public func close_window(&mut self, window : WindowId)
    @extern public func windows(&self) : &[WindowId]
    @extern public func window_size(&self, window : WindowId) : Option<Size>
    @extern public func set_window_size(&mut self, window : WindowId, size : Size)
    @extern public func focus_window(&mut self, window : WindowId)
    @extern public func set_window_title(&mut self, window : WindowId, title : string_view)
    @extern public func set_menu(&mut self, window : WindowId, menu : &[Menu]) : Result<Unit, UiError>
    @extern public func open_file_dialog(&mut self, opts : FileDialogOptions) : Result<Unit, UiError>
    @extern public func notify(&mut self, n : Notification) : Result<Unit, UiError>
    @extern public func set_tray(&mut self, menu : &[Menu]) : Result<Unit, UiError>
    @extern public func announce(&mut self, live : u8, text : string_view)
    @extern public func bind_shortcut(&mut self, s : Shortcut, cmd : CommandId) : Result<Unit, UiError>
    @extern public func unbind_shortcut(&mut self, s : Shortcut)
    @extern public func report_error(&mut self, key : Key, err : UiError)
}

// ── 6. Text ────────────────────────────────────────────────────────────────
@direct_init
public struct TextStyle {
    var family : FontFamily
    var family_name : string_view
    var size : float
    var weight : FontWeight
    var italic : bool
    var line_height : float
    var letter_spacing : float
    var features : TextFeatures
    var tab_size : u8
}
@direct_init
public struct TextFeatures {
    var ligatures : bool; var tabular_numbers : bool; var slashed_zero : bool
    var stylistic_set : u8; var raw_tags : &[u32]
}
@direct_init
public struct TextLayoutMetrics {
    var width : float; var height : float
    var first_baseline : float; var last_baseline : float
    var line_gap : float; var lines : u32; var truncated : bool
    var cap_height : float; var x_height : float
}
@direct_init
public struct TextConstraints {
    var max_width : float; var max_lines : u16
    var wrap : WrapMode; var overflow : TextOverflow
    var tab_size : u8; var direction : Direction
}
public struct TextContext {
    @extern public func measure(&self, text : string_view, style : TextStyle, c : TextConstraints) : TextLayoutMetrics
    @extern public func measure_rich(&self, spans : &[TextSpan], style : TextStyle, c : TextConstraints) : TextLayoutMetrics
}

// ── 7. Painter ─────────────────────────────────────────────────────────────
public struct Painter {
    @extern public func size(&self) : Size
    @extern public func theme(&self) : &Theme
    @extern public func color(&self, t : ColorToken) : Color
    @extern public func push_clip(&mut self, r : Rect, radius : float) : ClipGuard
    @extern public func push_opacity(&mut self, o : float) : LayerGuard
    @extern public func push_transform(&mut self, t : Transform2D) : TransformGuard
    @extern public func rect(&mut self, r : Rect, color : Color)
    @extern public func rounded_rect(&mut self, r : Rect, radius : float, color : Color)
    @extern public func border(&mut self, r : Rect, radius : float, width : float, color : Color)
    @extern public func line(&mut self, a : Point, b : Point, width : float, color : Color)
    @extern public func path(&mut self, path : &Path, color : Color)
    @extern public func gradient_rect(&mut self, r : Rect, g : Gradient)
    @extern public func shadow(&mut self, r : Rect, radius : float, s : Shadow)
    @extern public func image(&mut self, img : ImageHandle, r : Rect, fit : ImageFit)
    @extern public func text(&mut self, text : string_view, pos : Point, style : TextStyle, color : Color)
}

// ── 8. Semantics ───────────────────────────────────────────────────────────
public enum Role {
    Unknown, Button, Checkbox, Radio, Switch, Slider, TextField, MultilineTextField,
    Combobox, Listbox, Option, Spinbutton, SearchBox,
    Heading, Text, Image, Link, List, ListItem, Table, Grid, Row, Cell, ColumnHeader,
    Tree, TreeItem, Tab, TabList, TabPanel, Menu, MenuItem, MenuBar, MenuButton,
    Toolbar, Banner, Main, Navigation,
    Dialog, AlertDialog, Alert, Tooltip, ProgressBar, Status, Group, Form, Label,
    Separator, ScrollView, Canvas, Custom
}
@direct_init
public struct Semantics {
    var role : Role
    var label : string_view
    var description : string_view
    var value : string_view
    var alt : string_view
    var lang : string_view
    var placeholder : string_view
    var error : string_view
    var states : SemState
    var actions : SemAction
    var level : u8
    var pos_in_set : u32
    var set_size : u32
    var row_index : u32
    var column_index : u32
    var selected_index : i32
    var labelled_by : Option<NodeId>
    var described_by : Option<NodeId>
    var controls : Option<NodeId>
    var owns : Option<NodeId>
    var active_descendant : Option<NodeId>
    var has_popup : bool
    var secure : bool
}
@direct_init
public struct SemState {
    var focusable : bool; var focused : bool; var disabled : bool
    var checked : bool; var selected : bool; var expanded : bool
    var busy : bool; var invalid : bool; var readonly : bool
    var required : bool; var pressed : bool; var hidden : bool
    var modal : bool; var live : u8
    var multiselectable : bool
}
@direct_init
public struct SemAction {
    var press : bool; var focus : bool; var set_value : bool
    var scroll_into_view : bool; var expand : bool; var collapse : bool
    var increment : bool; var decrement : bool
}

// ── 9. Errors ──────────────────────────────────────────────────────────────
public enum ErrorClass { Expected, AppBug, Internal, Fatal }
public enum ErrorCode {
    Platform, Surface, Font, Image, Layout, InvalidKey, StaleNode, OutOfMemory,
    Protocol, Io, Timeout, NotImplemented, Unsupported, NotFound, PermissionDenied,
    InvalidArgument, Decode, ResourceExhausted, Cancelled, Busy, WouldBlock, DuplicateKey,
    StaleKey, ThreadAffinity, DeviceLost, Internal, Other
}
@direct_init
public struct UiError {
    private var code_value : ErrorCode
    private var class_value : ErrorClass
    private var detail : ErrorDetail
    private var source : Option<SourceLocation>
    private var retry : RetryHint
    @make @extern func make(code : ErrorCode, message : &'static str) : UiError
    @extern public func code(&self) : ErrorCode
    @extern public func class(&self) : ErrorClass
}

// ── 1.3 Backend selection ──────────────────────────────────────────────────
@extern public func gpu_info() : GpuInfo
@extern public func canvas_id(name : string_view) : Result<CanvasId, UiError>
@extern public func release_canvas(id : CanvasId)
@extern public func active_renderer() : Renderer
@extern public func force_renderer(r : Renderer) : Result<Unit, UiError>
@extern public func <A> run_headless(app : &mut A, options : Options) : Result<Unit, UiError> where A : Application

// ── 11. Test harness (ui_test) ─────────────────────────────────────────────
public struct UiHarness<'a, A> {
    @extern public func new(app : &'a mut A, options : Options) : Result<UiHarness<'a, A>, UiError> where A : Application
    @extern public func app(&self) : &A
    @extern public func app_mut(&mut self) : &mut A
    @extern public func size(&self) : Size
    @extern public func set_size(&mut self, w : float, h : float)
    @extern public func set_theme(&mut self, mode : ThemeMode)
    @extern public func set_reduced_motion(&mut self, on : bool)
    @extern public func set_scale(&mut self, scale : float)
    @extern public func frame(&mut self) : Result<FrameStats, UiError>
    @extern public func advance(&mut self, ms : double) : Result<FrameStats, UiError>
    @extern public func settle(&mut self) : Result<u32, UiError>
    @extern public func wait_until(&mut self, pred : (x : &A) => bool, timeout_ms : u32) : Result<Unit, UiError>
    @extern public func find_role(&self, role : Role) : Option<NodeId>
    @extern public func find_name(&self, name : string_view) : Option<NodeId>
    @extern public func find_test_id(&self, id : string_view) : Option<NodeId>
    @extern public func find_all(&self, q : Query) : &[NodeId]
    @extern public func within(&self, parent : NodeId, q : Query) : &[NodeId]
    @extern public func focused(&self) : Option<NodeId>
    @extern public func scroll_offset(&self, key : Key) : Point
    @extern public func by_key(&self, key : Key) : Option<NodeId>
    @extern public func semantics(&self, id : NodeId) : Option<&Semantics>
    @extern public func bounds(&self, id : NodeId) : Option<Rect>
    @extern public func text_of(&self, id : NodeId) : Option<&str>
    @extern public func click(&mut self, id : NodeId) : Result<Unit, UiError>
    @extern public func double_click(&mut self, id : NodeId) : Result<Unit, UiError>
    @extern public func hover(&mut self, id : NodeId) : Result<Unit, UiError>
    @extern public func drag(&mut self, from : NodeId, to : NodeId) : Result<Unit, UiError>
    @extern public func pointer(&mut self, kind : EventKind, x : float, y : float) : Result<Unit, UiError>
    @extern public func scroll(&mut self, id : NodeId, dx : float, dy : float) : Result<Unit, UiError>
    @extern public func key(&mut self, code : KeyCode, mods : Modifiers) : Result<Unit, UiError>
    @extern public func type_text(&mut self, text : string_view) : Result<Unit, UiError>
    @extern public func ime(&mut self, ev : ImeTestEvent) : Result<Unit, UiError>
    @extern public func screenshot(&self, path : string_view) : Result<Screenshot, UiError>
    @extern public func assert_golden(&self, path : string_view, opts : GoldenOptions) : Result<GoldenResult, UiError>
    @extern public func assert_semantics(&self, expected : &ComponentSpec) : Result<Unit, UiError>
    @extern public func assert_no_overflow(&self) : Result<Unit, UiError>
    @extern public func geometry_snapshot(&self) : string
    @extern public func semantics_snapshot(&self) : string
    @extern public func audit(&self) : AuditReport
}

} // namespace ui

public namespace ui_host {
    @extern public func <A> run(app : &mut A, argc : int, argv : **char) : int where A : ui::Application
    @extern public func <A> run_with(app : &mut A, options : ui::Options) : Result<Unit, ui::UiError> where A : ui::Application
}

public namespace ui_images {
    @extern public func image(path : string_view) : ui::ImageState
    @extern public func image_bytes(bytes : &[u8], hash : u64) : ui::ImageState
    @extern public func state(h : ui::ImageHandle) : ui::ImageState
    @extern public func release(h : ui::ImageHandle)
}

public namespace ui_icons {
    @extern public func find(name : string_view) : Option<ui::IconId>
    @extern public func list() : &[string_view]
}

public namespace ui_fonts {
    @extern public func system_stack() : ui::FontStack
    @extern public func add_file(path : string_view) : Result<ui::FontHandle, ui::FontError>
    @extern public func add_bytes(bytes : &[u8], name : string_view) : Result<ui::FontHandle, ui::FontError>
    @extern public func set_default(handle : ui::FontHandle)
    @extern public func default_handle() : ui::FontHandle
    @extern public func fetch(url : string_view, sha256 : string_view) : Result<ui::FontHandle, ui::FontError>
    @extern public func fetch_async(url : string_view, sha256 : string_view) : ui::FontRequest
    @extern public func cache_dir() : string_view
    @extern public func list_cached() : &[ui::FontCacheEntry]
    @extern public func use_inter_latin() : Result<ui::FontHandle, ui::FontError>
}

// Compile-only zeroability for constructor defaults. These values are never
// instantiated by this contract module.
unsafe impl Zeroable for ui::Options { }
unsafe impl Zeroable for ui::Limits { }
unsafe impl Zeroable for ui::FontStack { }
unsafe impl Zeroable for ui::GoldenOptions { }

// ── sample-only App + impl (not indexed; proves the interface) ─────────────
public struct App { }

impl ui::Application for App {
    public func on_start(&mut self, frame : &ui::FrameInfo, ctx : &mut ui::UiCtx) : Result<Unit, ui::UiError> {
        return Result.Ok<Unit, ui::UiError>(Unit{})
    }
    public func view(&self, frame : &ui::FrameInfo, b : &mut ui::ViewBuilder) : Result<ui::NodeId, ui::UiError> {
        return Result.Ok<ui::NodeId, ui::UiError>(zeroed<ui::NodeId>())
    }
    public func update(&mut self, ev : &ui::UiEvent, ctx : &mut ui::UiCtx) : Result<Unit, ui::UiError> {
        return Result.Ok<Unit, ui::UiError>(Unit{})
    }
    public func paint(&self, frame : &ui::FrameInfo, canvas : ui::CanvasPaint, p : &mut ui::Painter) : Result<Unit, ui::UiError> {
        return Result.Ok<Unit, ui::UiError>(Unit{})
    }
    public func on_close(&mut self, frame : &ui::FrameInfo, ctx : &mut ui::UiCtx) : ui::CloseAction {
        return ui::CloseAction.Allow
    }
}

public func main() : int {
    unsafe { write(1, "ui contract PASS\n" as *void, 17 as ulong) }
    return 0
}
