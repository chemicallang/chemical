// P1.8 display list and Painter recording tests. No rasterization.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using ui_internal::LayoutState;
using ui_internal::DisplayState;
using ui_internal::DisplayCommandKind;
using ui_internal::DisplayList;
using ui_internal::new_painter;
using ui_internal::display_build;
using ui_internal::display_command_count;
using ui_internal::display_kind;
using ui_internal::display_rect;
using ui_internal::display_text;
using ui_internal::display_transform;
using ui_internal::display_clip;
using ui_internal::display_opacity;
using ui_internal::display_layer;
using ui_internal::display_z;
using ui_internal::display_baseline;
using ui_internal::display_path_count;
using ui_internal::display_gradient_count;
using ui_internal::display_damage;
using ui_internal::display_serialize;
using ui_internal::RenderBackend;
using ui_internal::RecordingBackend;
using ui_internal::new_layout_state;
using ui_internal::new_layout_context;
using ui_internal::layout_run;
using ui_internal::new_view_builder;
using ui_internal::view_root;
using ui_internal::empty_text_context;

public namespace display_tests {
func close(a : float, b : float) : bool {
    var d = a - b
    if(d < 0.0f) { d = 0.0f - d }
    return d <= 0.5f
}
func fail(name : *char) : bool {
    unsafe { printf("display FAIL %s\n", name) }
    return false
}
func build_node_list(b : &ViewBuilder, st : &LayoutState, theme : &Theme) : u32 {
    var ds = DisplayState.new(512u32)
    var _=display_build(&mut ds,b,st,theme,&empty_text_context(),Size.new(200.0f,100.0f))
    return display_command_count(&ds)
}
func node_commands() : bool {
    var b=new_view_builder(64u32,200.0f,100.0f); var root=view_root(&b)
    var box=b.row(key("box"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)).background(ColorToken.Primary).foreground(ColorToken.OnPrimary).border(Border{width:2.0f,color:ColorToken.Danger}).radius(Radius.SM).shadow(Elevation.Low).opacity(0.5f)); b.child(root,box)
    var t=b.text(key("t"),"A",Style.new().text(TextRole.Body)); b.child(box,t)
    var img=b.image(key("im"),ImageSource.Path("test.png"),Style.new().width(Dim.Px(8.0f)).height(Dim.Px(8.0f))); b.child(box,img)
    var ic=b.icon(key("ic"),IconId.zero(),Style.new().width(Dim.Px(8.0f)).height(Dim.Px(8.0f))); b.child(box,ic)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),200.0f,100.0f); var ls=new_layout_state(64u32); var _=layout_run(&mut ls,&b,&ctx,&theme)
    var ds=DisplayState.new(512u32); var _n=display_build(&mut ds,&b,&ls,&theme,&empty_text_context(),Size.new(200.0f,100.0f))
    var n=display_command_count(&ds)
    if(n < 8u32) { return fail("command-count") }
    if(display_kind(&ds,0u32) != DisplayCommandKind.TransformPush && display_kind(&ds,0u32) != DisplayCommandKind.LayerPush) {
        // Shadow is first if present; layer/transform may precede.
    }
    var has_shadow=false; var has_round=false; var has_border=false; var has_text=false; var has_image=false; var has_icon=false
    var i=0u32
    while(i<n) {
        var k=display_kind(&ds,i)
        if(k == DisplayCommandKind.Shadow) { has_shadow=true }
        if(k == DisplayCommandKind.RoundedRect) { has_round=true }
        if(k == DisplayCommandKind.Border) { has_border=true }
        if(k == DisplayCommandKind.Text) { has_text=true }
        if(k == DisplayCommandKind.Image) { has_image=true }
        if(k == DisplayCommandKind.Icon) { has_icon=true }
        i=i+1u32
    }
    if(!has_shadow || !has_round || !has_border || !has_text || !has_image || !has_icon) { return fail("command-kinds") }
    // Determinism: rebuilding into a second state yields the same count/order.
    var ds2=DisplayState.new(512u32); var _n2=display_build(&mut ds2,&b,&ls,&theme,&empty_text_context(),Size.new(200.0f,100.0f))
    if(display_command_count(&ds2) != n) { return fail("determinism-count") }
    i=0u32; while(i<n) { if(display_kind(&ds2,i) != display_kind(&ds,i)) { return fail("determinism-order") }; i=i+1u32 }
    return true
}
func clip_transform_layers() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var sc=b.scroll(key("sc"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)),Axis.Vertical); b.child(root,sc)
    var c=b.row(key("c"),Style.new().width(Dim.Px(20.0f)).height(Dim.Px(20.0f)).background(ColorToken.Surface)); b.child(sc,c)
    b.transform(c,Transform2D.translate(5.0f,6.0f))
    var ov=b.overlay(key("ov"),OverlayLayer.Dialog,Option.Some(root)); b.child(root,ov); b.style(ov,Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f)).background(ColorToken.Danger).z(7i32))
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var ls=new_layout_state(64u32); var _=layout_run(&mut ls,&b,&ctx,&theme)
    var ds=DisplayState.new(512u32); var _n=display_build(&mut ds,&b,&ls,&theme,&empty_text_context(),Size.new(100.0f,100.0f))
    var clip_found=false; var tr_found=false; var layer_found=false; var i=0u32
    while(i<display_command_count(&ds)) {
        var k=display_kind(&ds,i)
        if(k == DisplayCommandKind.ClipPush) { clip_found=true }
        if(k == DisplayCommandKind.TransformPush) { tr_found=true }
        if(k == DisplayCommandKind.LayerPush) { if(display_layer(&ds,i) == OverlayLayer.Dialog as u8) { layer_found=true } }
        i=i+1u32
    }
    if(!clip_found || !tr_found || !layer_found) { return fail("stack-commands") }
    return true
}
func painter_records() : bool {
    var theme=Theme.light(); var ds=DisplayState.new(256u32)
    var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(100.0f,50.0f))
    p.rect(Rect.new(0.0f,0.0f,10.0f,10.0f),Color.new(1u8,2u8,3u8))
    p.rounded_rect(Rect.new(1.0f,1.0f,10.0f,10.0f),2.0f,Color.new(1u8,1u8,1u8))
    p.border(Rect.new(0.0f,0.0f,10.0f,10.0f),2.0f,1.0f,Color.new(0u8,0u8,0u8))
    p.line(Point{x:0.0f,y:0.0f},Point{x:10.0f,y:10.0f},1.0f,Color.new(0u8,0u8,0u8))
    var path=Path{commands:vector<PathCommand>(),fill_rule:0u8}; path.commands.push(PathCommand.MoveTo(Point{x:0.0f,y:0.0f})); path.commands.push(PathCommand.LineTo(Point{x:1.0f,y:1.0f})); p.path(&path,Color.new(0u8,0u8,0u8))
    var stops=vector<GradientStop>(); stops.push(GradientStop{offset:0.0f,color:Color.new(0u8,0u8,0u8)}); stops.push(GradientStop{offset:1.0f,color:Color.new(255u8,255u8,255u8)})
    p.gradient_rect(Rect.new(0.0f,0.0f,10.0f,10.0f),Gradient.Linear(Point{x:0.0f,y:0.0f},Point{x:10.0f,y:0.0f},stops))
    p.shadow(Rect.new(0.0f,0.0f,10.0f,10.0f),2.0f,Shadow{x:0.0f,y:1.0f,blur:2.0f,spread:0.0f,color:Color.new(0u8,0u8,0u8)})
    p.image(ImageHandle.zero(),Rect.new(0.0f,0.0f,10.0f,10.0f),ImageFit.None)
    p.text("A",Point{x:0.0f,y:10.0f},theme.text(TextRole.Body),Color.new(0u8,0u8,0u8))
    {
        var g=p.push_clip(Rect.new(0.0f,0.0f,50.0f,50.0f),0.0f)
        p.rect(Rect.new(0.0f,0.0f,5.0f,5.0f),Color.new(0u8,0u8,0u8))
    }
    {
        var g=p.push_opacity(0.5f)
        p.rect(Rect.new(0.0f,0.0f,5.0f,5.0f),Color.new(0u8,0u8,0u8))
    }
    {
        var g=p.push_transform(Transform2D.translate(1.0f,2.0f))
        p.rect(Rect.new(0.0f,0.0f,5.0f,5.0f),Color.new(0u8,0u8,0u8))
    }
    if(display_command_count(&ds) < 16u32 || display_path_count(&ds) != 1u32 || display_gradient_count(&ds) != 1u32) { return fail("painter-commands") }
    return true
}
func build_10k() : ViewBuilder {
    var b=new_view_builder(10064u32,200000.0f,100.0f); var root=view_root(&b)
    var row=b.row(key("r"),Style.new().width(Dim.Px(200000.0f)).height(Dim.Px(20.0f))); b.child(root,row)
    var i=0u32
    while(i<10000u32) { var c=b.row(key_of("c",i as u64),Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f)).background(ColorToken.Primary)); b.child(row,c); i=i+1u32 }
    return b
}
func is_llvm() : bool { var e=environment::get("CHEMICAL_UI_TEST_BACKEND"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("llvm",4u)}; return false }
func is_release() : bool { var e=environment::get("CHEMICAL_UI_RELEASE"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("1",1u)}; return false }
func sorted5(a:*mut i64) { var i=0; while(i<5){var j=i+1; while(j<5){ if(unsafe(a[j])<unsafe(a[i])){var t=unsafe(a[i]);unsafe{a[i]=unsafe(a[j]);a[j]=t}}; j=j+1}; i=i+1} }
func perf_display() : bool {
    var b=build_10k(); var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),200000.0f,100.0f); var ls=new_layout_state(10064u32); var _=layout_run(&mut ls,&b,&ctx,&theme)
    var times : [5]i64
    var i=0
    while(i<5) {
        var ds=DisplayState.new(10064u32)
        var start=std::chrono::Instant.now()
        var _n=display_build(&mut ds,&b,&ls,&theme,&empty_text_context(),Size.new(200000.0f,100.0f))
        times[i]=start.elapsed().as_micros()
        i=i+1
    }
    sorted5(&raw mut times[0])
    unsafe { printf("display 10k_us p50=%ld p95=%ld\n", times[2], times[4]) }
    if(is_llvm() && (times[2] > 3000i64 || times[4] > 7000i64)) { return fail("display-gate") }
    return true
}
func backend_damage() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var box=b.row(key("box"),Style.new().width(Dim.Px(20.0f)).height(Dim.Px(20.0f)).background(ColorToken.Primary)); b.child(root,box)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var ls=new_layout_state(64u32); var _=layout_run(&mut ls,&b,&ctx,&theme)
    var ds=DisplayState.new(128u32); var _n=display_build(&mut ds,&b,&ls,&theme,&empty_text_context(),Size.new(100.0f,100.0f))
    var dmg=display_damage(&ds,Size.new(100.0f,100.0f),false)
    if(dmg.size() == 0u) { return fail("damage-empty") }
    var backend=RecordingBackend.make()
    var sub=backend.submit(&ds.list,dmg.as_span())
    if(sub is Result.Err || backend.submits != 1u32 || backend.last_commands == 0u32 || backend.last_damage.size() == 0u32) { return fail("backend-submit") }
    var pres=backend.present()
    if(pres is Result.Err || backend.presents != 1u32) { return fail("backend-present") }
    backend.resize(100u32,100u32); backend.device_lost()
    if(backend.resizes != 1u32 || backend.lost != 1u32) { return fail("backend-hooks") }
    if(display_damage(&ds,Size.new(100.0f,100.0f),true).size() != 1u32) { return fail("damage-full") }
    return true
}
func scene_build(id : u32) : DisplayState {
    var b=new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var bg = ColorToken.Primary
    if(id % 3u == 0u) { bg = ColorToken.Secondary }
    if(id % 3u == 1u) { bg = ColorToken.Success }
    var box=b.row(key("scene"),Style.new().width(Dim.Px(80.0f)).height(Dim.Px(60.0f)).background(bg).border(Border{width:1.0f,color:ColorToken.Border}).radius(Radius.SM)); b.child(root,box)
    if(id % 2u == 0u) { var t=b.text(key("t"),"Scene",Style.new().text(TextRole.Body)); b.child(box,t) }
    if(id % 4u == 0u) { var im=b.image(key("im"),ImageSource.Path("scene.png"),Style.new().width(Dim.Px(8.0f)).height(Dim.Px(8.0f))); b.child(box,im) }
    if(id % 5u == 0u) { var ov=b.overlay(key("ov"),OverlayLayer.Dialog,Option.Some(root)); b.child(root,ov); b.style(ov,Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f)).background(ColorToken.Danger).z((id as i32))) }
    if(id % 6u == 0u) { b.transform(box,Transform2D.translate(1.0f,2.0f)) }
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var ls=new_layout_state(64u32); var _=layout_run(&mut ls,&b,&ctx,&theme)
    var ds=DisplayState.new(256u32); var _n=display_build(&mut ds,&b,&ls,&theme,&empty_text_context(),Size.new(100.0f,100.0f))
    if(id % 7u == 0u) {
        var th=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut th,Size.new(100.0f,100.0f))
        var path=Path{commands:vector<PathCommand>(),fill_rule:0u8}; path.commands.push(PathCommand.MoveTo(Point{x:0.0f,y:0.0f})); path.commands.push(PathCommand.LineTo(Point{x:5.0f,y:5.0f})); p.path(&path,Color.new(1u8,2u8,3u8))
        var stops=vector<GradientStop>(); stops.push(GradientStop{offset:0.0f,color:Color.new(0u8,0u8,0u8)}); stops.push(GradientStop{offset:1.0f,color:Color.new(255u8,255u8,255u8)})
        p.gradient_rect(Rect.new(0.0f,0.0f,10.0f,10.0f),Gradient.Linear(Point{x:0.0f,y:0.0f},Point{x:10.0f,y:0.0f},stops))
    }
    return ds
}
func scenes() : bool {
    var update=false
    var e=environment::get("CHEMICAL_UI_UPDATE_GOLDENS")
    if(e is Option.Some) { var Some(v)=e else return false; update = v.to_view().equals_with_len("1",1u) }
    var i=0u32
    while(i<20u32) {
        var ds=scene_build(i); var text=display_serialize(&ds)
        var full = string("lang/tests/ui/fixtures/display/scene")
        if(i < 9u32) { full.append('0') }
        full.append_integer((i + 1u32) as int)
        full.append_view(".dl.txt")
        var p=path::PathBuf.from(std::str.make_view(full.to_view()))
        if(update) { var wr=fs::write(&p.as_path(),text.to_view().as_bytes()); if(wr is Result.Err){return fail("scene-write")} }
        else {
            var rd=fs::read_to_string(&p.as_path())
            if(rd is Result.Err){ return fail("scene-read") }
            var Ok(want)=rd else return false
            if(!want.to_view().equals(&text.to_view())){ return fail("scene-mismatch") }
        }
        i=i+1u32
    }
    return true
}
func run_display() : bool {
    if(!node_commands()) { return false }
    if(!clip_transform_layers()) { return false }
    if(!painter_records()) { return false }
    if(!backend_damage()) { return false }
    if(!scenes()) { return false }
    if(is_release()) { if(!perf_display()) { return false } }
    return true
}
}
