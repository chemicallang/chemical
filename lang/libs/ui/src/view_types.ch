// P1.3 supporting value types for ViewBuilder. No layout/paint behavior.
using std::Option;
using std::Result;
using std::Unit;
using std::string_view;

public enum Direction { Auto, Ltr, Rtl }
public enum Breakpoint { Sm, Md, Lg, Xl }
public enum Overflow { Visible, Clip, Scroll }
public enum Align { Start, Center, End, Stretch, Baseline }
public enum Justify { Start, Center, End, SpaceBetween, SpaceAround, SpaceEvenly }
public enum Axis { Horizontal, Vertical, Both }
public enum TextAlign { Start, Center, End, Justify }
public enum WrapMode { NoWrap, Wrap, WrapReverse }
public enum TextOverflow { Clip, Ellipsis }
public enum WidgetState { Hover, Focus, Pressed, Disabled, Selected, Checked, Invalid }
public enum OverlayLayer { Tooltip, Popover, Menu, Dialog, Toast }
public enum Easing { Linear, EaseIn, EaseOut, EaseInOut, Spring }
public enum AnimProperty { Opacity, TranslateX, TranslateY, Scale, Rotate, Background, Width, Height }
public enum Cursor { Arrow, IBeam, Hand, Wait, Crosshair, Move, ResizeNS, ResizeEW, ResizeNESW, ResizeNWSE, NotAllowed }
public enum ImageFit { Contain, Cover, Fill, None }

public enum Role {
    Unknown, Button, Checkbox, Radio, Switch, Slider, TextField, MultilineTextField,
    Combobox, Listbox, Option, Spinbutton, SearchBox,
    Heading, Text, Image, Link, List, ListItem, Table, Grid, Row, Cell, ColumnHeader,
    Tree, TreeItem, Tab, TabList, TabPanel, Menu, MenuItem, MenuBar, MenuButton,
    Toolbar, Banner, Main, Navigation,
    Dialog, AlertDialog, Alert, Tooltip, ProgressBar, Status, Group, Form, Label,
    Separator, ScrollView, Canvas, Custom
}

public struct WindowId {
    private var index_value : u32
    private var generation_value : u32
}

public struct CanvasId {
    private var value : u32
    func zero() : CanvasId { return zeroed<CanvasId>() }
}
public struct IconId {
    private var value : u32
    func zero() : IconId { return zeroed<IconId>() }
}
public struct ImageHandle {
    private var index_value : u32
    private var generation_value : u32
    func zero() : ImageHandle { return zeroed<ImageHandle>() }
}
public struct FontHandle {
    private var index_value : u32
    private var generation_value : u32
    func zero() : FontHandle { return zeroed<FontHandle>() }
}
public struct CommandId {
    private var value : u32
    func zero() : CommandId { return zeroed<CommandId>() }
}

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

public variant Dim {
    Auto()
    Px(value : float)
    Percent(value : float)
    Fill()
}
public variant Position { Relative() Absolute(top : Dim, right : Dim, bottom : Dim, left : Dim) }
public variant ScrollAnchor { Start() End() Center() Node(key : Key) }
public variant GridTrack { Px(value : float) Fr(value : float) Auto() }
public variant ImageSource {
    Path(path : string_view)
    Bytes(bytes : &[u8])
    Handle(h : ImageHandle)
    Placeholder()
}

@direct_init
public struct Border { var width : float; var color : ColorToken }

@direct_init
public struct TextSpan {
    var text : string_view
    var role : TextRole
    var color : ColorToken
    var weight : FontWeight
    var italic : bool
    var family : FontFamily
    var features : TextFeatures
    var link : Option<string_view>
    var decoration : u8
}

@direct_init
public struct GridTracks {
    var columns : &[GridTrack]
    var rows : &[GridTrack]
    var gap : Space
}

@direct_init
public struct AnimationSpec {
    var duration : Duration
    var easing : Easing
    var delay : Duration
    var repeat : u32
    var alternate : bool
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

public struct Component {
    internal var root : NodeId
    internal var key_value : Key
    internal var name_value : string_view
}

