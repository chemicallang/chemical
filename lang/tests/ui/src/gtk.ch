// P1.14 GTK host unit/input mapping and real-window smoke.
using std::Option;
using std::string_view;
using std::Result;
using std::Unit;

public namespace gtk_tests {

func fail(name : *char) : bool { unsafe { printf("gtk FAIL %s\n", name) }; return false }
func mods() : Modifiers { return Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false } }

public struct DemoApp {
    var toggled : bool
    var clicks : u32
    var updates : u32
}
impl Application for DemoApp {
    public func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId,UiError> {
        var root=ui_internal::view_root(b)
        b.style(root,Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
        var col : ColorToken = ColorToken.Danger
        if(self.toggled) { col = ColorToken.Primary }
        var btn=b.row(key("button"),Style.new().width(Dim.Px(160.0f)).height(Dim.Px(80.0f)).background(col).radius(Radius.MD))
        b.focusable(btn,0i32); b.test_id(btn,"toggle"); b.child(root,btn)
        var other=b.row(key("other"),Style.new().width(Dim.Px(80.0f)).height(Dim.Px(44.0f)).background(ColorToken.SurfaceRaised)); b.child(root,other)
        return Result.Ok<NodeId,UiError>(root)
    }
    public func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit,UiError> {
        self.updates=self.updates+1u32
        if(ev.kind == EventKind.Release && ev.local_x >= 0.0f && ev.local_x <= 160.0f && ev.local_y >= 0.0f && ev.local_y <= 80.0f) {
            self.toggled=!self.toggled; self.clicks=self.clicks+1u32; ctx.request_view()
        }
        return Result.Ok<Unit,UiError>(Unit{})
    }
    public func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction { return CloseAction.Allow }
}

func mapping() : bool {
    var ev=window::Event.make(); ev.kind=window::EVENT_MOUSE_DOWN; ev.x=100; ev.y=50; ev.button=window::MOUSE_LEFT
    var m=ui_native::map_window_event(&ev,2.0f)
    if(m.is_none()){return fail("map-none")}
    var ue=ui_native::option_event(m, UiEvent { window:zeroed<WindowId>(),key:zeroed<Key>(),kind:EventKind.Frame,payload:Payload.None(),modifiers:mods(),local_x:0.0f,local_y:0.0f,timestamp_ms:0.0,frame:0u64,pointer_button:0u8,scroll_dx:0.0f,scroll_dy:0.0f,key_code:KeyCode.Unknown,key_repeat:false,text_value:"" })
    if(ue.kind != EventKind.Press){return fail("map-kind")}
    if(ue.local_x != 50.0f || ue.local_y != 25.0f){return fail("map-scale")}
    var key=window::Event.make(); key.kind=window::EVENT_KEY_DOWN; key.key=0xFF1B
    var km=ui_native::map_window_event(&key,1.0f)
    var ke=ui_native::option_event(km, ue)
    if(ke.key_code != KeyCode.Escape){return fail("map-escape")}
    return true
}

func have_display() : bool {
    var d=environment::get("DISPLAY")
    if(d.is_some()) { switch(d) { Some(v) => { if(v.size() > 0u) { return true } } None() => { } } }
    var w=environment::get("WAYLAND_DISPLAY")
    if(w.is_some()) { switch(w) { Some(v) => { if(v.size() > 0u) { return true } } None() => { } } }
    return false
}
func update_goldens() : bool {
    var e=environment::get("CHEMICAL_UI_UPDATE_GTK")
    if(e.is_none()){return false}
    switch(e) { Some(v) => { return v.to_view().equals_with_len("1",1u) } None() => { return false } }
}
func png_dim(data : &[u8], at : size_t) : u32 {
    if(at + 3u >= data.size()) { return 0u }
    var a=unsafe(data.data()[at]) as u32; var b=unsafe(data.data()[at+1u]) as u32
    var c=unsafe(data.data()[at+2u]) as u32; var d=unsafe(data.data()[at+3u]) as u32
    return (a << 24u) | (b << 16u) | (c << 8u) | d
}
func real_smoke() : bool {
    if(!have_display()) { unsafe { printf("gtk BLOCKED no DISPLAY/WAYLAND_DISPLAY\n") }; return true }
    var app=DemoApp { toggled:false, clicks:0u32, updates:0u32 }
    var opts=Options.make(); opts.width=320; opts.height=240; opts.headless=false; opts.debug=true; opts.fixed_time=true
    var update=update_goldens()
    var first:string_view="/tmp/ui_gtk_before.png"; var click:string_view="/tmp/ui_gtk_after_click.png"; var resize:string_view="/tmp/ui_gtk_after_resize.png"
    if(update) { first="lang/tests/ui/fixtures/gtk/before.png"; click="lang/tests/ui/fixtures/gtk/after_click.png"; resize="lang/tests/ui/fixtures/gtk/after_resize.png" }
    ui_native::set_gtk_test_plan(ui_native::GtkTestPlan {
        click_after_ms:120u32, click_x:70, click_y:70, direct_click:true,
        resize_after_ms:260u32, resize_w:420, resize_h:300,
        quit_after_ms:520u32, idle_ms:0u32,
        first_png:first, click_png:click, resize_png:resize
    })
    var r=run_with(&mut app,opts)
    ui_native::clear_gtk_test_plan()
    if(r is Result.Err){return fail("real-run")}
    if(app.clicks != 1u32 || !app.toggled){return fail("real-click")}
    var fp=path::PathBuf.from(first); var cp=path::PathBuf.from(click); var zp=path::PathBuf.from(resize)
    var f=fs::read(&fp.as_path()); if(f is Result.Err){return fail("real-first-png")}
    var c=fs::read(&cp.as_path()); if(c is Result.Err){return fail("real-click-png")}
    var z=fs::read(&zp.as_path()); if(z is Result.Err){return fail("real-resize-png")}
    var Ok(fd)=f else return false; var Ok(cd)=c else return false; var Ok(zd)=z else return false
    var bw=png_dim(fd.as_span(),16u); var bh=png_dim(fd.as_span(),20u)
    var cw=png_dim(cd.as_span(),16u); var ch=png_dim(cd.as_span(),20u)
    var zw=png_dim(zd.as_span(),16u); var zh=png_dim(zd.as_span(),20u)
    if(bw == 0u || bh == 0u || cw == 0u || ch == 0u || zw == 0u || zh == 0u){return fail("real-png-header")}
    if(bw != cw || bh != ch){return fail("real-click-size")}
    if(zw >= bw){return fail("real-resize-size")}
    unsafe { printf("gtk real PASS clicks=%d updates=%d size=%dx%d resize=%dx%d\n", app.clicks as int, app.updates as int, bw as int, bh as int, zw as int, zh as int) }
    return true
}
func run_gtk_tests() : bool {
    if(!mapping()){return false}
    if(!real_smoke()){return false}
    return true
}
}
