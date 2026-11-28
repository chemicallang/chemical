// P1.13 typed-error boundary, fault injection and panic policy.
using std::Option;
using std::Result;
using std::Unit;
using std::string;
using std::string_view;
using std::vector;

public namespace error_tests {

func efail(name : *char) : bool { unsafe { printf("errors FAIL %s\n", name) }; return false }
func mods() : Modifiers { return Modifiers { shift : false, ctrl : false, alt : false, meta : false, caps : false } }
func opts(w : i32, h : i32, frames : u32) : Options {
    var o=Options.make(); o.width=w; o.height=h; o.max_frames=frames; o.fixed_time=true; o.headless=true; o.theme=ThemeMode.Light; o.scale=1.0f; o.font_file="lang/tests/ui/assets/fonts/LiberationSans-Regular.ttf"; return o
}
public struct FaultApp {
    var fail_start : u32
    var fail_view : u32
    var fail_update : u32
    var view_calls : u32
    var update_calls : u32
    var err_seen : u32
    var injected : bool
}
impl Application for FaultApp {
    public func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit,UiError> {
        if(self.fail_start > 0u32) { self.fail_start = self.fail_start - 1u32; return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Font, "start failure")) }
        return Result.Ok<Unit,UiError>(Unit{})
    }
    public func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId,UiError> {
        // view is &self; failure count is driven through the immutable value at creation
        if(self.fail_view > 0u32) { return Result.Err<NodeId,UiError>(UiError.make(ErrorCode.Layout, "view failure")) }
        var root=ui_internal::view_root(b)
        var box=b.row(key("box"),Style.new().width(Dim.Px(120.0f)).height(Dim.Px(44.0f)).background(ColorToken.Primary)); b.focusable(box,0i32); b.child(root,box)
        var cv=b.canvas(key("canvas"),Style.new().width(Dim.Px(32.0f)).height(Dim.Px(32.0f)),CanvasId.zero()); b.child(root,cv)
        return Result.Ok<NodeId,UiError>(root)
    }
    public func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit,UiError> {
        self.update_calls = self.update_calls + 1u32
        if(self.fail_update > 0u32) { self.fail_update = self.fail_update - 1u32; return Result.Err<Unit,UiError>(UiError.make(ErrorCode.Image, "update failure")) }
        return Result.Ok<Unit,UiError>(Unit{})
    }
    public func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction { return CloseAction.Allow }
}
public struct ErrorViewApp {
    var use_b_error : bool
    var use_report : bool
    var view_calls : u32
}
impl Application for ErrorViewApp {
    public func on_start(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : Result<Unit,UiError> {
        if(self.use_report) { ctx.report_error(key("reported"), UiError.make(ErrorCode.InvalidArgument, "reported failure")) }
        return Result.Ok<Unit,UiError>(Unit{})
    }
    public func view(&self, frame : &FrameInfo, b : &mut ViewBuilder) : Result<NodeId,UiError> {
        var root=ui_internal::view_root(b)
        var box=b.row(key("bad"),Style.new().width(Dim.Px(44.0f)).height(Dim.Px(44.0f)).background(ColorToken.Primary)); b.focusable(box,0i32)
        if(self.use_b_error) { b.error(box, UiError.make(ErrorCode.Image, "decode failure")) }
        b.child(root,box)
        return Result.Ok<NodeId,UiError>(root)
    }
    public func update(&mut self, ev : &UiEvent, ctx : &mut UiCtx) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func paint(&self, frame : &FrameInfo, canvas : CanvasPaint, p : &mut Painter) : Result<Unit,UiError> { return Result.Ok<Unit,UiError>(Unit{}) }
    public func on_close(&mut self, frame : &FrameInfo, ctx : &mut UiCtx) : CloseAction { return CloseAction.Allow }
}
func survives(app : &mut FaultApp, frames : u32) : bool {
    var r=run_headless(app, opts(200,160,frames)); return r is Result.Ok
}
func fault_matrix() : bool {
    var sites : [8]u8 = [1u8,2u8,3u8,4u8,5u8,6u8,2u8,3u8]
    var codes : [8]ErrorCode = [ErrorCode.OutOfMemory, ErrorCode.Image, ErrorCode.Font, ErrorCode.Layout, ErrorCode.Protocol, ErrorCode.NotFound, ErrorCode.StaleNode, ErrorCode.StaleKey]
    var i=0u32
    while(i<8u32) {
        var code=ErrorCode.Image
        if(i<8u32){code=codes[i]}
        headless_fault_set(sites[i],code,1u32)
        var app=FaultApp { fail_start:0u32, fail_view:0u32, fail_update:0u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:true }
        if(!survives(&mut app,4u32)){headless_fault_clear();return efail("fault-survive")}
        headless_fault_clear()
        i=i+1u32
    }
    return true
}
func callback_failures() : bool {
    var a=FaultApp { fail_start:1u32, fail_view:0u32, fail_update:0u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:false }
    if(!survives(&mut a,2u32)){return efail("start-boundary")}
    var b=FaultApp { fail_start:0u32, fail_view:1u32, fail_update:0u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:false }
    if(!survives(&mut b,2u32)){return efail("view-boundary")}
    var c=FaultApp { fail_start:0u32, fail_view:0u32, fail_update:1u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:false }
    if(!survives(&mut c,3u32)){return efail("update-boundary")}
    return true
}
func b_error_and_report() : bool {
    var a=ErrorViewApp { use_b_error:true, use_report:false, view_calls:0u32 }
    if(run_headless(&mut a,opts(200,160,2u32)) is Result.Err){return efail("b-error")}
    var b=ErrorViewApp { use_b_error:false, use_report:true, view_calls:0u32 }
    if(run_headless(&mut b,opts(200,160,2u32)) is Result.Err){return efail("report-error")}
    return true
}
func retry_check() : bool {
    var a=FaultApp { fail_start:0u32, fail_view:0u32, fail_update:0u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:false }
    headless_input_reset()
    headless_fault_set(2u8,ErrorCode.Layout,1u32)
    // Error-card retry lives at x=60,y=40 in the 360x180 card.
    headless_input_pointer(EventKind.Release,60.0f,40.0f,0u8,mods())
    var r=run_headless(&mut a,opts(320,200,4u32))
    headless_fault_clear()
    if(r is Result.Err){return efail("retry-run")}
    return true
}
func paint_boundary() : bool {
    var a=FaultApp { fail_start:0u32, fail_view:0u32, fail_update:0u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:false }
    headless_fault_set(6u8,ErrorCode.Image,1u32)
    var ok=survives(&mut a,2u32)
    var left=headless_fault_remaining()
    headless_fault_clear()
    if(!ok || left != 0u32){return efail("paint-boundary")}
    return true
}
func fatal_check() : bool {
    var e=UiError.make(ErrorCode.Internal,"x")
    if(e.class() == ErrorClass.Fatal){return efail("forged-fatal")}
    var o=UiError.make(ErrorCode.Other,"x")
    if(o.class() == ErrorClass.Fatal){return efail("other-fatal")}
    return true
}

func glyph_rows(c : u8) : [7]u8 {
    switch(c) {
        'A' => return [14u8,17u8,17u8,31u8,17u8,17u8,17u8]
        'E' => return [31u8,16u8,16u8,30u8,16u8,16u8,31u8]
        'F' => return [31u8,16u8,16u8,30u8,16u8,16u8,16u8]
        'I' => return [31u8,4u8,4u8,4u8,4u8,4u8,31u8]
        'L' => return [16u8,16u8,16u8,16u8,16u8,16u8,31u8]
        'O' => return [14u8,17u8,17u8,17u8,17u8,17u8,14u8]
        'R' => return [30u8,17u8,17u8,30u8,20u8,18u8,17u8]
        'T' => return [31u8,4u8,4u8,4u8,4u8,4u8,4u8]
        'U' => return [17u8,17u8,17u8,17u8,17u8,17u8,14u8]
        'V' => return [17u8,17u8,17u8,17u8,17u8,10u8,4u8]
        'W' => return [17u8,17u8,17u8,21u8,21u8,21u8,10u8]
        'Y' => return [17u8,10u8,4u8,4u8,4u8,4u8,4u8]
        ' ' => return [0u8,0u8,0u8,0u8,0u8,0u8,0u8]
        default => return [0u8,0u8,0u8,0u8,0u8,0u8,0u8]
    }
}
func draw_label(px : &mut [u8], w : u32, x : u32, y : u32, text : string_view, c : Color) {
    var cx=x; var i:size_t=0u
    while(i < text.size()) {
        var oc=text.get(i)
        if(oc.is_some()) {
            var ch : u8 = 0u8
            switch(oc) { Some(v) => { ch = v as u8 } None() => { } }
            if(ch >= 97u8 && ch <= 122u8) { ch = ch - 32u8 }
            var rows=glyph_rows(ch); var ry=0u32
            while(ry<7u32) {
                var rx=0u32
                while(rx<5u32) {
                    if((rows[ry] & (1u8 << (4u32-rx))) != 0u8) {
                        var idx:size_t=((y+ry) as size_t * (w as size_t) + (cx+rx) as size_t) * 4u
                        if(idx+3u < px.size()) { px[idx]=c.r; px[idx+1u]=c.g; px[idx+2u]=c.b; px[idx+3u]=c.a }
                    }
                    rx=rx+1u32
                }
                ry=ry+1u32
            }
        }
        cx=cx+6u32; i=i+1u
    }
}
func annotate_error_card(update : bool) : bool {
    var rp=path::PathBuf.from("/tmp/ui_error_card_runtime.png")
    var ap=path::PathBuf.from("/tmp/ui_error_card.png")
    var rd=fs::read(&rp.as_path()); if(rd is Result.Err){return efail("card-base-read")}
    var Ok(data)=rd else return false
    var dec=image::Image.from_png(data.as_span()); if(dec is Result.Err){return efail("card-base-decode")}
    var Ok(img)=dec else return false
    var w=img.width() as u32
    var px=img.pixels_mut()
    var ink=Color.new(15u8,23u8,42u8); var light=Color.new(255u8,255u8,255u8)
    draw_label(px,w,16u32,16u32,"UI ERROR",ink)
    draw_label(px,w,16u32,28u32,"VIEW FAILURE",ink)
    draw_label(px,w,20u32,48u32,"RETRY",light)
    var enc=img.encode_png(); if(enc is Result.Err){return efail("card-encode")}
    var Ok(bytes)=enc else return false
    var wr=fs::write(&ap.as_path(),bytes.as_span()); if(wr is Result.Err){return efail("card-write")}
    if(update) {
        var fp=path::PathBuf.from("lang/tests/ui/fixtures/errors/error_card.png")
        var fw=fs::write(&fp.as_path(),bytes.as_span()); if(fw is Result.Err){return efail("card-fixture-write")}
    }
    return true
}
func error_card_png(update : bool) : bool {
    var a=FaultApp { fail_start:0u32, fail_view:4u32, fail_update:0u32, view_calls:0u32, update_calls:0u32, err_seen:0u32, injected:false }
    var p=path::PathBuf.from("lang/tests/ui/fixtures/errors/error_card.png")
    var runtime=path::PathBuf.from("/tmp/ui_error_card_runtime.png")
    var r=run_headless_shot(&mut a,opts(320,200,2u32),runtime.as_view())
    if(r is Result.Err){return efail("card-shot")}
    if(!annotate_error_card(update)){return false}
    if(update){return true}
    var annotated=path::PathBuf.from("/tmp/ui_error_card.png")
    var ra=fs::read(&annotated.as_path()); var rb=fs::read(&p.as_path())
    if(ra is Result.Err || rb is Result.Err){return efail("card-read")}
    var Ok(da)=ra else return false; var Ok(db)=rb else return false
    if(da.size()!=db.size()){return efail("card-size")}
    var i:size_t=0u; while(i<da.size()){if(unsafe(da.get_ptr(i)[0])!=unsafe(db.get_ptr(i)[0])){return efail("card-golden")};i=i+1u}
    return true
}

func run_error_tests() : bool {
    var update=false; var e=environment::get("CHEMICAL_UI_UPDATE_GOLDENS"); if(e is Option.Some){var Some(v)=e else return false; update=v.to_view().equals_with_len("1",1u)}
    if(!fault_matrix()){return false}
    if(!callback_failures()){return false}
    if(!b_error_and_report()){return false}
    if(!retry_check()){return false}
    if(!paint_boundary()){return false}
    if(!fatal_check()){return false}
    if(!error_card_png(update)){return false}
    return true
}
}
