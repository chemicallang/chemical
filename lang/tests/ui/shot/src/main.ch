using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;

public struct ShotApp { }
impl Application for ShotApp {
    public func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit, UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId, UiError> {
        var root = ui_internal::view_root(b)
        var canvas = b.row(key("canvas"), Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
        b.child(root, canvas)
        var a = b.row(key("a"), Style.new().width(Dim.Px(120.0f)).height(Dim.Px(80.0f)).background(ColorToken.Primary).radius(Radius.MD))
        var c = b.row(key("c"), Style.new().width(Dim.Px(90.0f)).height(Dim.Px(60.0f)).background(ColorToken.Danger).radius(Radius.SM))
        var d = b.row(key("d"), Style.new().width(Dim.Px(160.0f)).height(Dim.Px(50.0f)).background(ColorToken.Success).radius(Radius.LG))
        b.child(canvas, a); b.child(canvas, c); b.child(canvas, d)
        return Result.Ok<NodeId,UiError>(root)
    }
    public func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit, UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit, UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction { return CloseAction.Allow }
}
func env(name : &str) : string {
    var v = environment::get(name)
    if(v is Option.Some) { var Some(s) = v else return string(""); return s }
    return string("")
}
public func main() : int {
    var out = env("CHEMICAL_UI_SHOT_OUT")
    if(out.size() == 0u) { unsafe { write(2, "ui_shot: missing CHEMICAL_UI_SHOT_OUT\n" as *void, 39u) }; return 2 }
    var o = Options.make()
    o.headless = true
    o.width = 320
    o.height = 200
    o.max_frames = 1u32
    o.fixed_time = true
    o.scale = 1.0f
    o.font_file = "lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf"
    var theme_name = env("CHEMICAL_UI_SHOT_THEME")
    if(theme_name.equals_with_len("dark", 4u)) { o.theme = ThemeMode.Dark }
    else if(theme_name.equals_with_len("high-contrast", 13u)) { o.theme = ThemeMode.HighContrast }
    else { o.theme = ThemeMode.Light }
    var scale_text = env("CHEMICAL_UI_SHOT_SCALE")
    if(scale_text.size() > 0u) {
        var f = scale_text.to_float()
        if(f is Result.Ok) { var Ok(sv) = f else return 2; o.scale = sv }
    }
    var app = ShotApp{}
    var r = run_headless_shot(&mut app, o, out.to_view())
    if(r is Result.Err) { unsafe { write(2, "ui_shot: render failed\n" as *void, 24u) }; return 1 }
    unsafe { write(1, "ui_shot: wrote PNG\n" as *void, 19u) }
    return 0
}
