// P1.12 semantics tree, audit core, snapshots and overlay.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using ui_internal::SemanticsTree;
using ui_internal::SemanticsNode;
using ui_internal::semantics_empty;
using ui_internal::semantics_build;
using ui_internal::semantics_count;
using ui_internal::semantics_find_role;
using ui_internal::semantics_find_name;
using ui_internal::semantics_find_test_id;
using ui_internal::semantics_bounds;
using ui_internal::semantics_focused;
using ui_internal::semantics_snapshot;
using ui_internal::semantics_audit;
using ui_internal::audit_passed;
using ui_internal::audit_finding_count;
using ui_internal::audit_finding;
using ui_internal::audit_error_count;
using ui_internal::semantics_overlay_build;
using ui_internal::semantics_role_name;
using ui_internal::semantics_node_at;
using ui_internal::raster_set_pixel;
using ui_internal::semantics_diff;
using ui_internal::DisplayState;
using ui_internal::new_view_builder;
using ui_internal::new_layout_state;
using ui_internal::new_layout_context;
using ui_internal::layout_run;
using ui_internal::display_build;
using ui_internal::Rasterizer;
using ui_internal::ColorBuffer;
using ui_internal::raster_save_png;
using ui_internal::empty_text_context;

public namespace semantics_tests {

func fail(name : *char) : bool { unsafe { printf("semantics FAIL %s\n", name) }; return false }
func sem(role : Role, label : string_view) : Semantics {
    var s = semantics_empty(); s.role = role; s.label = label; return s
}
func button_scene(b : &mut ViewBuilder) {
    var root = ui_internal::view_root(b)
    var col = b.column(key("col"), Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface))
    b.child(root, col)
    var btn = b.row(key("btn"), Style.new().width(Dim.Px(88.0f)).height(Dim.Px(44.0f)).background(ColorToken.Surface).foreground(ColorToken.OnSurface).radius(Radius.MD))
    b.focusable(btn, 0i32); b.test_id(btn, "save"); b.semantics(btn, sem(Role.Button, "Save")); b.child(col, btn)
    var link = b.row(key("link"), Style.new().width(Dim.Px(80.0f)).height(Dim.Px(44.0f)).background(ColorToken.Surface).foreground(ColorToken.OnSurface))
    b.focusable(link, 0i32); b.semantics(link, sem(Role.Link, "Open")); b.child(col, link)
    var h = b.text(key("heading"), "Welcome", Style.new().text(TextRole.H2)); b.test_id(h, "title")
    var hs = sem(Role.Heading, "Welcome"); hs.level = 1u8; b.semantics(h, hs); b.child(col, h)
    var body = b.text(key("body"), "Body copy", Style.new().text(TextRole.Body)); b.child(col, body)
    var hid = b.text(key("hidden"), "Hidden", Style.new().text(TextRole.Body)); var hsem = sem(Role.Text, "Hidden"); hsem.states.hidden = true; b.semantics(hid, hsem); b.child(col, hid)
}
func build(w : float, h : float) : SemanticsTree {
    var b = new_view_builder(128u32, w, h); button_scene(&mut b); var theme=Theme.light(); var lc=new_layout_context(empty_text_context(),w,h); var ls=new_layout_state(128u32); var _=layout_run(&mut ls,&b,&lc,&theme)
    return semantics_build(&b,&ls,&theme,Size.new(w,h),Option.Some<Key>(key("btn")))
}
func tree_checks() : bool {
    var t = build(320.0f, 200.0f)
    var sn = semantics_snapshot(&t)
    if(semantics_count(&t) != 4u32) { return fail("count") }
    var r = semantics_find_role(&t, Role.Button)
    if(r is Option.None) { return fail("find-role") }
    var n = semantics_find_name(&t, "Save")
    if(n is Option.None) { return fail("find-name") }
    var ti = semantics_find_test_id(&t, "save")
    if(ti is Option.None) { return fail("find-test") }
    var Some(btn) = ti else return false
    var bb = semantics_bounds(&t, btn)
    if(bb is Option.None) { return fail("bounds") }
    var focused = semantics_focused(&t)
    if(focused is Option.None) { return fail("focused") }
    var snap = semantics_snapshot(&t)
    if(snap.size() == 0u || !snap.contains("Button")) { return fail("snapshot") }
    var theme=Theme.light(); var bo=new_view_builder(32u32,200.0f,160.0f); var oroot=ui_internal::view_root(&bo)
    var base=bo.row(key("base"),Style.new().width(Dim.Px(80.0f)).height(Dim.Px(44.0f)).background(ColorToken.Surface)); bo.focusable(base,0i32); bo.semantics(base,sem(Role.Button,"Base")); bo.child(oroot,base)
    var ov=bo.overlay(key("ov"),OverlayLayer.Dialog,Option.Some<NodeId>(oroot)); bo.style(ov,Style.new().width(Dim.Px(80.0f)).height(Dim.Px(44.0f)).background(ColorToken.Danger)); bo.focusable(ov,0i32); bo.semantics(ov,sem(Role.Button,"Overlay")); bo.child(oroot,ov)
    var lo=new_layout_state(32u32); var lco=new_layout_context(empty_text_context(),200.0f,160.0f); var _o=layout_run(&mut lo,&bo,&lco,&theme); var to=semantics_build(&bo,&lo,&theme,Size.new(200.0f,160.0f),Option.None<Key>())
    var bkey=semantics_find_name(&to,"Base"); var okey=semantics_find_name(&to,"Overlay")
    if(bkey is Option.None || okey is Option.None) { return fail("overlay-missing") }
    var bi=-1; var oi=-1; var j=0u32
    while(j<semantics_count(&to)){var sn2=semantics_node_at(&to,j); if(sn2.semantics.label.equals_with_len("Base",4u)){bi=j as int}; if(sn2.semantics.label.equals_with_len("Overlay",7u)){oi=j as int}; j=j+1u32}
    if(bi < 0 || oi <= bi) { return fail("overlay-order") }
    return true
}
func increment_scene(b : &mut ViewBuilder, add : bool) {
    var root = ui_internal::view_root(b)
    var col = b.column(key("col"), Style.new().width(Dim.Fill()).height(Dim.Fill())); b.child(root,col)
    var a = b.row(key("a"), Style.new().width(Dim.Px(50.0f)).height(Dim.Px(44.0f)).background(ColorToken.Primary)); b.focusable(a,0i32); b.semantics(a,sem(Role.Button,"A")); b.child(col,a)
    if(add) { var c = b.row(key("c"), Style.new().width(Dim.Px(50.0f)).height(Dim.Px(44.0f)).background(ColorToken.Danger)); b.focusable(c,0i32); b.semantics(c,sem(Role.Button,"C")); b.child(col,c) }
}
func diff_checks() : bool {
    var b1=new_view_builder(32u32,100.0f,100.0f); increment_scene(&mut b1,false); var theme=Theme.light(); var lc=new_layout_context(empty_text_context(),100.0f,100.0f); var l1=new_layout_state(32u32); var _=layout_run(&mut l1,&b1,&lc,&theme); var t1=semantics_build(&b1,&l1,&theme,Size.new(100.0f,100.0f),Option.None<Key>())
    var b2=new_view_builder(32u32,100.0f,100.0f); increment_scene(&mut b2,true); var l2=new_layout_state(32u32); var _2=layout_run(&mut l2,&b2,&lc,&theme); var t2=semantics_build(&b2,&l2,&theme,Size.new(100.0f,100.0f),Option.None<Key>())
    var d=semantics_diff(&t1,&t2)
    var added=false; var i=0u32
    while(i<d.size()) { var u=ui_internal::semantics_update_at(&d,i); if(u.change == SemanticsChange.Add) { added=true }; i=i+1u32 }
    if(!added) { return fail("diff-add") }
    return true
}
func bad_scene(b : &mut ViewBuilder) {
    var root=ui_internal::view_root(b)
    var col=b.column(key("col"),Style.new().width(Dim.Fill()).height(Dim.Fill()).background(ColorToken.Surface)); b.child(root,col)
    var small=b.row(key("small"),Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f)).background(ColorToken.Primary)); b.focusable(small,0i32); b.semantics(small,sem(Role.Button,"")); b.test_id(small,"dup"); b.child(col,small)
    var disabled=b.row(key("dis"),Style.new().width(Dim.Px(44.0f)).height(Dim.Px(44.0f))); b.focusable(disabled,0i32); b.disabled(disabled,true); b.semantics(disabled,sem(Role.Button,"Disabled")); b.child(col,disabled)
    var img=b.image(key("img"),ImageSource.Placeholder(),Style.new().width(Dim.Px(44.0f)).height(Dim.Px(44.0f))); var ims=sem(Role.Image,""); b.semantics(img,ims); b.child(col,img)
    var h=b.text(key("h"),"Heading",Style.new().text(TextRole.H3)); var hs=sem(Role.Heading,"Heading"); hs.level=3u8; b.semantics(h,hs); b.child(col,h)
    var low=b.text(key("low"),"Low",Style.new().text(TextRole.Body).foreground(ColorToken.Surface)); b.semantics(low,sem(Role.Text,"Low")); b.child(col,low)
    var sec=b.text(key("sec"),"secret",Style.new().text(TextRole.Body)); var ss=sem(Role.TextField,"Secret"); ss.secure=true; ss.value="secret"; b.semantics(sec,ss); b.child(col,sec)
    var dup=b.row(key("dup"),Style.new().width(Dim.Px(44.0f)).height(Dim.Px(44.0f)).background(ColorToken.Primary)); b.focusable(dup,0i32); b.semantics(dup,sem(Role.Button,"Duplicate")); b.test_id(dup,"dup"); b.child(col,dup)
    var dupkey=b.row(key("dup"),Style.new().width(Dim.Px(44.0f)).height(Dim.Px(44.0f))); b.focusable(dupkey,0i32); b.semantics(dupkey,sem(Role.Button,"DuplicateKey")); b.child(col,dupkey)
}
func has_code(codes : &vector<string_view>, code : string_view) : bool {
    var i : size_t = 0u
    while(i < codes.size()) { var c = unsafe(codes.get_ptr(i)[0]); if(c.equals(&code)) { return true }; i=i+1u }
    return false
}
func audit_checks() : bool {
    var b=new_view_builder(128u32,320.0f,200.0f); bad_scene(&mut b); var theme=Theme.light(); var lc=new_layout_context(empty_text_context(),320.0f,200.0f); var ls=new_layout_state(128u32); var _=layout_run(&mut ls,&b,&lc,&theme); var t=semantics_build(&b,&ls,&theme,Size.new(320.0f,200.0f),Option.None<Key>())
    var r=semantics_audit(&t,AuditOptions.make())
    if(r.passed) { return fail("audit-false-green") }
    if(audit_error_count(&r) < 4u32) { return fail("audit-errors") }
    var codes=vector<string_view>()
    var i=0u32
    while(i<audit_finding_count(&r)) { var f=audit_finding(&r,i); codes.push(f.code); i=i+1u32 }
    if(!has_code(&codes, "UI-A11Y-001")) { return fail("audit-name") }
    if(!has_code(&codes, "UI-A11Y-005")) { return fail("audit-target") }
    if(!has_code(&codes, "UI-A11Y-007")) { return fail("audit-contrast") }
    if(!has_code(&codes, "UI-A11Y-008")) { return fail("audit-alt") }
    if(!has_code(&codes, "UI-A11Y-009")) { return fail("audit-secure") }
    if(!has_code(&codes, "UI-A11Y-010")) { return fail("audit-heading") }
    if(!has_code(&codes, "UI-A11Y-012")) { return fail("audit-testid") }
    if(!has_code(&codes, "UI-A11Y-011")) { return fail("audit-key") }
    var good=build(320.0f,200.0f); var gr=semantics_audit(&good,AuditOptions.make())
    if(!gr.passed) { return fail("audit-good") }
    return true
}
func snapshot_check(update : bool) : bool {
    var t=build(320.0f,200.0f); var snap=semantics_snapshot(&t)
    var p=path::PathBuf.from("lang/tests/ui/fixtures/semantics/snapshot.json")
    if(update) { var w=fs::write(&p.as_path(),snap.to_view().as_bytes()); if(w is Result.Err){return fail("snapshot-write")}; return true }
    var rd=fs::read_to_string(&p.as_path()); if(rd is Result.Err){return fail("snapshot-read")}
    var Ok(want)=rd else return false
    if(!want.to_view().equals(snap.to_view())) { return fail("snapshot-golden") }
    return true
}
func glyph_rows(c : u8) : [7]u8 {
    switch(c) {
        'A' => return [14u8,17u8,17u8,31u8,17u8,17u8,17u8]
        'B' => return [30u8,17u8,17u8,30u8,17u8,17u8,30u8]
        'C' => return [15u8,16u8,16u8,16u8,16u8,16u8,15u8]
        'D' => return [30u8,17u8,17u8,17u8,17u8,17u8,30u8]
        'E' => return [31u8,16u8,16u8,30u8,16u8,16u8,31u8]
        'F' => return [31u8,16u8,16u8,30u8,16u8,16u8,16u8]
        'G' => return [15u8,16u8,16u8,23u8,17u8,17u8,14u8]
        'H' => return [17u8,17u8,17u8,31u8,17u8,17u8,17u8]
        'I' => return [31u8,4u8,4u8,4u8,4u8,4u8,31u8]
        'J' => return [7u8,2u8,2u8,2u8,2u8,18u8,12u8]
        'K' => return [17u8,18u8,20u8,24u8,20u8,18u8,17u8]
        'L' => return [16u8,16u8,16u8,16u8,16u8,16u8,31u8]
        'M' => return [17u8,27u8,21u8,21u8,17u8,17u8,17u8]
        'N' => return [17u8,25u8,21u8,19u8,17u8,17u8,17u8]
        'O' => return [14u8,17u8,17u8,17u8,17u8,17u8,14u8]
        'P' => return [30u8,17u8,17u8,30u8,16u8,16u8,16u8]
        'Q' => return [14u8,17u8,17u8,17u8,21u8,18u8,13u8]
        'R' => return [30u8,17u8,17u8,30u8,20u8,18u8,17u8]
        'S' => return [15u8,16u8,16u8,14u8,1u8,1u8,30u8]
        'T' => return [31u8,4u8,4u8,4u8,4u8,4u8,4u8]
        'U' => return [17u8,17u8,17u8,17u8,17u8,17u8,14u8]
        'V' => return [17u8,17u8,17u8,17u8,17u8,10u8,4u8]
        'W' => return [17u8,17u8,17u8,21u8,21u8,21u8,10u8]
        'X' => return [17u8,10u8,4u8,4u8,4u8,10u8,17u8]
        'Y' => return [17u8,10u8,4u8,4u8,4u8,4u8,4u8]
        'Z' => return [31u8,1u8,2u8,4u8,8u8,16u8,31u8]
        default => return [0u8,0u8,0u8,0u8,0u8,0u8,0u8]
    }
}
func draw_label(buf : *mut ColorBuffer, x : u32, y : u32, text : string_view, c : Color) {
    var cx = x
    var i : size_t = 0u
    while(i < text.size()) {
        var oc = text.get(i)
        if(oc.is_some()) {
            var ch = oc.take_unchecked() as u8
            if(ch >= 97u8 && ch <= 122u8) { ch = ch - 32u8 }
            var rows = glyph_rows(ch); var ry = 0u32
            while(ry < 7u32) { var rx = 0u32; while(rx < 5u32) { if((rows[ry] & (1u8 << (4u32 - rx))) != 0u8) { raster_set_pixel(buf,cx+rx,y+ry,c) }; rx=rx+1u32 }; ry=ry+1u32 }
        }
        cx = cx + 6u32; i = i + 1u
    }
}
func overlay_check(update : bool) : bool {
    var t=build(320.0f,200.0f); var theme=Theme.light(); var ds=semantics_overlay_build(&t,&theme)
    var buf=ColorBuffer.new(320u32,200u32); var r=Rasterizer.new(&raw mut buf); var rs=r.render(&ds.list)
    var ink=Color.new(15u8,23u8,42u8)
    var ni=0u32
    while(ni<semantics_count(&t)) {
        var sn=semantics_node_at(&t,ni)
        var role=semantics_role_name(sn.semantics.role)
        var lx=sn.bounds.x as u32 + 4u32; var ly=sn.bounds.y as u32 + 4u32
        draw_label(&raw mut buf,lx,ly,role,ink)
        draw_label(&raw mut buf,lx,ly+9u32,sn.semantics.label,ink)
        ni=ni+1u32
    }

    var p=path::PathBuf.from("lang/tests/ui/fixtures/semantics/overlay.png")
    var w=path::PathBuf.from("/tmp/ui_semantics_overlay.png")
    var out=w; if(update){out=p}
    var sv=raster_save_png(&buf,&out.as_path()); if(sv is Result.Err){return fail("overlay-save")}
    if(update){return true}
    var ra=fs::read(&w.as_path()); var rb=fs::read(&p.as_path()); if(ra is Result.Err || rb is Result.Err){return fail("overlay-read")}
    var Ok(da)=ra else return false; var Ok(db)=rb else return false
    if(da.size()!=db.size()){return fail("overlay-size")}
    var i:size_t=0u; while(i<da.size()){if(unsafe(da.get_ptr(i)[0])!=unsafe(db.get_ptr(i)[0])){return fail("overlay-golden")};i=i+1u}
    return true
}
func build_10k() : bool {
    var b=new_view_builder(10016u32,10000.0f,20.0f); var root=ui_internal::view_root(&b); var row=b.row(key("row"),Style.new().width(Dim.Px(10000.0f)).height(Dim.Px(20.0f))); b.child(root,row)
    var i=0u32; while(i<10000u32){var n=b.row(key_of("n",i as u64),Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f))); b.focusable(n,0i32); b.child(row,n); i=i+1u32}
    var theme=Theme.light(); var lc=new_layout_context(empty_text_context(),10000.0f,20.0f); var ls=new_layout_state(10016u32); var _=layout_run(&mut ls,&b,&lc,&theme)
    var times:[5]i64; var t=0
    while(t<5){var s=std::chrono::Instant.now(); var _t=semantics_build(&b,&ls,&theme,Size.new(10000.0f,20.0f),Option.None<Key>()); times[t]=s.elapsed().as_micros(); t=t+1}
    var a=0; while(a<5){var j=a+1; while(j<5){if(times[j]<times[a]){var q=times[a];times[a]=times[j];times[j]=q};j=j+1};a=a+1}
    unsafe { printf("semantics 10k_us p50=%ld p95=%ld\n",times[2],times[4]) }
    return true
}
func run_semantics_tests() : bool {
    var update=false; var e=environment::get("CHEMICAL_UI_UPDATE_GOLDENS"); if(e is Option.Some){var Some(v)=e else return false; update=v.to_view().equals_with_len("1",1u)}
    if(!tree_checks()){return false}
    if(!diff_checks()){return false}
    if(!audit_checks()){return false}
    if(!snapshot_check(update)){return false}
    if(!overlay_check(update)){return false}
    if(is_release()){if(!build_10k()){return false}}
    return true
}
func is_release() : bool { var e=environment::get("CHEMICAL_UI_RELEASE"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("1",1u)}; return false }
}
