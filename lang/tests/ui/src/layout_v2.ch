// P1.7 layout v2: wrap, grid, scroll, layers.
using std::Option;
using std::Result;
using std::string_view;
using std::vector;
using ui_internal::new_view_builder;
using ui_internal::view_root;
using ui_internal::reset_view_builder;
using ui_internal::new_layout_state;
using ui_internal::new_layout_context;
using ui_internal::LayoutState;
using ui_internal::LayoutContext;
using ui_internal::layout_run;
using ui_internal::layout_rect;
using ui_internal::layout_scroll_offset;
using ui_internal::layout_set_scroll;
using ui_internal::layout_scroll_to;
using ui_internal::layout_scroll_content_size;
using ui_internal::layout_scroll_max;
using ui_internal::layout_scroll_offset_of;
using ui_internal::layout_scrollbar_visible_x;
using ui_internal::layout_scrollbar_visible_y;
using ui_internal::layout_scrollbar_thumb_x;
using ui_internal::layout_scrollbar_thumb_y;
using ui_internal::layout_clip;
using ui_internal::layout_scroll_damage;
using ui_internal::layout_overlay_layer;
using ui_internal::layout_z;
using ui_internal::layout_is_overlay_node;
using ui_internal::layout_line_index;
using ui_internal::empty_text_context;

public namespace layout_v2_tests {

func close(a : float, b : float) : bool {
    var d = a - b
    if(d < 0.0f) { d = 0.0f - d }
    return d <= 0.5f
}
func fail(name : *char) : bool {
    unsafe { printf("layout_v2 FAIL %s\n", name) }
    return false
}
func get_rect(st : &LayoutState, id : NodeId) : Rect {
    var r = layout_rect(st, id)
    if(r is Option.Some) { var Some(v)=r else return Rect.zero(); return v }
    return Rect.zero()
}
func box(b : &mut ViewBuilder, k : Key, w : float, h : float, parent : NodeId) : NodeId {
    var n = b.row(k, Style.new().width(Dim.Px(w)).height(Dim.Px(h)))
    b.child(parent,n)
    return n
}
func wrap_case() : bool {
    var b = new_view_builder(64u32, 100.0f, 100.0f); var root=view_root(&b)
    var w = b.wrap(key("wrap"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)).wrap(WrapMode.Wrap)); b.child(root,w)
    var a=box(&mut b,key("a"),30.0f,10.0f,w)
    var c=box(&mut b,key("c"),30.0f,10.0f,w)
    var d=box(&mut b,key("d"),30.0f,10.0f,w)
    var e=box(&mut b,key("e"),30.0f,10.0f,w)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    var _=layout_run(&mut st,&b,&ctx,&theme)
    if(!close(get_rect(&st,a).y,0.0f) || !close(get_rect(&st,c).y,0.0f) || !close(get_rect(&st,d).y,0.0f) || !close(get_rect(&st,e).y,10.0f)) { return fail("wrap-lines") }
    if(layout_line_index(&st,e) != 1u32) { return fail("wrap-line-index") }
    var br = new_view_builder(64u32,100.0f,100.0f); var rr=view_root(&br)
    var rw = br.wrap(key("rw"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)).wrap(WrapMode.WrapReverse)); br.child(rr,rw)
    var ra=box(&mut br,key("a"),30.0f,10.0f,rw); var rc=box(&mut br,key("c"),30.0f,10.0f,rw); var rd=box(&mut br,key("d"),30.0f,10.0f,rw); var re=box(&mut br,key("e"),30.0f,10.0f,rw)
    var t2=Theme.light(); var c2=new_layout_context(empty_text_context(),100.0f,100.0f); var s2=new_layout_state(64u32); var _2=layout_run(&mut s2,&br,&c2,&t2)
    if(!close(get_rect(&s2,ra).y,10.0f) || !close(get_rect(&s2,re).y,0.0f)) { return fail("wrap-reverse") }
    return true
}
func grid_case() : bool {
    var b = new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var cols=vector<GridTrack>(); cols.push(GridTrack.Px(50.0f)); cols.push(GridTrack.Fr(1.0f))
    var rows=vector<GridTrack>(); rows.push(GridTrack.Px(20.0f)); rows.push(GridTrack.Fr(1.0f))
    var tracks=GridTracks { columns : cols.as_span(), rows : rows.as_span(), gap : Space.None }
    var grid=b.grid(key("grid"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)), tracks); b.child(root,grid)
    var a=box(&mut b,key("a"),1.0f,1.0f,grid); b.grid_cell(a,0u16,0u16,1u16,1u16)
    var c=box(&mut b,key("c"),1.0f,1.0f,grid); b.grid_cell(c,1u16,0u16,1u16,1u16)
    var d=box(&mut b,key("d"),1.0f,1.0f,grid); b.grid_cell(d,0u16,1u16,1u16,1u16)
    var span=box(&mut b,key("span"),1.0f,1.0f,grid); b.grid_cell(span,0u16,0u16,2u16,1u16)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    var _=layout_run(&mut st,&b,&ctx,&theme)
    if(!close(get_rect(&st,a).w,50.0f) || !close(get_rect(&st,c).w,50.0f) || !close(get_rect(&st,d).h,80.0f) || !close(get_rect(&st,span).w,100.0f)) { return fail("grid-tracks") }
    return true
}
func grid_auto_span() : bool {
    var b = new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var cols=vector<GridTrack>(); cols.push(GridTrack.Auto()); cols.push(GridTrack.Fr(1.0f))
    var rows=vector<GridTrack>(); rows.push(GridTrack.Auto())
    var tracks=GridTracks { columns : cols.as_span(), rows : rows.as_span(), gap : Space.None }
    var grid=b.grid(key("grid"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)), tracks); b.child(root,grid)
    var a=box(&mut b,key("a"),30.0f,10.0f,grid); b.grid_cell(a,0u16,0u16,1u16,1u16)
    var c=box(&mut b,key("c"),20.0f,20.0f,grid); b.grid_cell(c,1u16,0u16,1u16,1u16)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    var _=layout_run(&mut st,&b,&ctx,&theme)
    if(!close(get_rect(&st,a).w,30.0f) || !close(get_rect(&st,c).w,70.0f) || !close(get_rect(&st,c).h,20.0f)) { return fail("grid-auto") }
    return true
}
func scroll_case() : bool {
    var b = new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var sc=b.scroll(key("sc"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)), Axis.Vertical); b.child(root,sc)
    var content=b.column(key("content"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(200.0f))); b.child(sc,content)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    layout_set_scroll(&mut st, key("sc"), Point { x : 0.0f, y : 50.0f })
    var _=layout_run(&mut st,&b,&ctx,&theme)
    var content_rect=get_rect(&st,content)
    if(!close(content_rect.y,-50.0f) || !close(layout_scroll_offset_of(&st,sc).y,50.0f)) { return fail("scroll-offset") }
    var sz=layout_scroll_content_size(&st,sc)
    var mx=layout_scroll_max(&st,sc)
    if(!close(sz.height,200.0f) || !close(mx.y,150.0f) || !layout_scrollbar_visible_y(&st,sc)) { return fail("scroll-metrics") }
    var clip=layout_clip(&st,sc)
    if(clip is Option.Some) { var Some(cr)=clip else return false; if(!close(cr.h,50.0f)) { return fail("scroll-clip") } } else { return fail("scroll-clip-missing") }
    if(layout_scrollbar_thumb_y(&st,sc).h <= 0.0f) { return fail("scroll-thumb") }
    layout_set_scroll(&mut st,key("sc"),Point{x:0.0f,y:100.0f})
    var _up=layout_run(&mut st,&b,&ctx,&theme)
    if(!close(get_rect(&st,content).y,-100.0f) || !close(layout_scroll_offset_of(&st,sc).y,100.0f)) { return fail("scroll-update") }
    // Anchor end and scalar keyed state.
    var b2=new_view_builder(64u32,100.0f,100.0f); var r2=view_root(&b2)
    var s2=b2.scroll(key("end"), Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)), Axis.Vertical, ScrollAnchor.End()); b2.child(r2,s2)
    var ct=b2.column(key("ct"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(200.0f))); b2.child(s2,ct)
    var t2=Theme.light(); var c2=new_layout_context(empty_text_context(),100.0f,100.0f); var st2=new_layout_state(64u32); var _2=layout_run(&mut st2,&b2,&c2,&t2)
    if(!close(layout_scroll_offset_of(&st2,s2).y,150.0f) || !close(get_rect(&st2,ct).y,-150.0f)) { return fail("scroll-anchor-end") }
    layout_scroll_to(&mut st2,key("end"),ScrollAnchor.Start())
    var _3=layout_run(&mut st2,&b2,&c2,&t2)
    if(!close(layout_scroll_offset_of(&st2,s2).y,0.0f)) { return fail("scroll-to") }
    return true
}
func scroll_axes_case() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var sh=b.scroll(key("h"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)),Axis.Horizontal); b.child(root,sh)
    var ch=b.column(key("ch"),Style.new().width(Dim.Px(200.0f)).height(Dim.Px(50.0f))); b.child(sh,ch)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    layout_set_scroll(&mut st,key("h"),Point{x:30.0f,y:0.0f}); var _=layout_run(&mut st,&b,&ctx,&theme)
    if(!close(get_rect(&st,ch).x,-30.0f) || !close(layout_scroll_max(&st,sh).x,100.0f) || !layout_scrollbar_visible_x(&st,sh)) { return fail("scroll-horizontal") }
    var b2=new_view_builder(64u32,100.0f,100.0f); var r2=view_root(&b2)
    var sb=b2.scroll(key("b"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)),Axis.Both); b2.child(r2,sb)
    var cb=b2.column(key("cb"),Style.new().width(Dim.Px(200.0f)).height(Dim.Px(200.0f))); b2.child(sb,cb)
    var c2=new_layout_context(empty_text_context(),100.0f,100.0f); var s2=new_layout_state(64u32); var _2=layout_run(&mut s2,&b2,&c2,&theme)
    if(!close(layout_scroll_max(&s2,sb).x,100.0f) || !close(layout_scroll_max(&s2,sb).y,150.0f) || !layout_scrollbar_visible_x(&s2,sb) || !layout_scrollbar_visible_y(&s2,sb)) { return fail("scroll-both") }
    layout_set_scroll(&mut s2,key("b"),Point{x:500.0f,y:500.0f}); var _3=layout_run(&mut s2,&b2,&c2,&theme)
    var co=layout_scroll_offset_of(&s2,sb); if(!close(co.x,100.0f) || !close(co.y,150.0f)) { return fail("scroll-clamp") }
    var b3=new_view_builder(64u32,100.0f,100.0f); var r3=view_root(&b3)
    var ss=b3.scroll(key("small"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)),Axis.Vertical); b3.child(r3,ss)
    var cs=b3.column(key("cs"),Style.new().width(Dim.Px(50.0f)).height(Dim.Px(20.0f))); b3.child(ss,cs)
    var c3=new_layout_context(empty_text_context(),100.0f,100.0f); var s3=new_layout_state(64u32); var _4=layout_run(&mut s3,&b3,&c3,&theme)
    if(layout_scrollbar_visible_y(&s3,ss) || !close(layout_scroll_max(&s3,ss).y,0.0f)) { return fail("scroll-small") }
    return true
}
func nested_scroll_case() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var outer=b.scroll(key("outer"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(50.0f)),Axis.Vertical); b.child(root,outer)
    var inner=b.scroll(key("inner"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)),Axis.Vertical); b.child(outer,inner)
    var content=b.column(key("c"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(200.0f))); b.child(inner,content)
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    layout_set_scroll(&mut st,key("outer"),Point{x:0.0f,y:20.0f}); layout_set_scroll(&mut st,key("inner"),Point{x:0.0f,y:30.0f})
    var _=layout_run(&mut st,&b,&ctx,&theme)
    var ir=get_rect(&st,inner); var cr=get_rect(&st,content)
    if(!close(ir.y,-20.0f) || !close(cr.y,-50.0f)) { return fail("nested-scroll") }
    return true
}
func layers_case() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var root=view_root(&b)
    var normal=box(&mut b,key("normal"),10.0f,10.0f,root)
    var ov=b.overlay(key("ov"),OverlayLayer.Dialog,Option.Some(root)); b.child(root,ov); b.style(ov,Style.new().width(Dim.Px(20.0f)).height(Dim.Px(10.0f)).z(5i32))
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    var _=layout_run(&mut st,&b,&ctx,&theme)
    if(!layout_is_overlay_node(&st,ov) || layout_overlay_layer(&st,ov) != OverlayLayer.Dialog || layout_z(&st,ov) != 5i32) { return fail("layer-meta") }
    var nr=get_rect(&st,normal); var orr=get_rect(&st,ov)
    if(!close(nr.x,0.0f) || !close(nr.y,0.0f) || !close(orr.w,20.0f) || !close(orr.h,10.0f)) { return fail("layer-flow") }
    return true
}
func build_wrap(n : u32) : ViewBuilder {
    var b=new_view_builder(n+64u32,100000.0f,100000.0f); var root=view_root(&b)
    var w=b.wrap(key("w"),Style.new().width(Dim.Px(1000.0f)).height(Dim.Px(100000.0f)).wrap(WrapMode.Wrap)); b.child(root,w)
    var i=0u32
    while(i<n) {
        if(i%5u==0u) { var t=b.text(key_of("t",i as u64),"A",Style.new().text(TextRole.Body)); b.child(w,t) }
        else { var x=b.row(key_of("b",i as u64),Style.new().width(Dim.Px(10.0f)).height(Dim.Px(10.0f))); b.child(w,x) }
        i=i+1u32
    }
    return b
}
func build_grid(n : u32) : ViewBuilder {
    var b=new_view_builder(n+64u32,100.0f,100.0f); var root=view_root(&b)
    var cols=vector<GridTrack>(); var rows=vector<GridTrack>()
    var k=0u32; while(k<100u32) { cols.push(GridTrack.Px(1.0f)); rows.push(GridTrack.Px(1.0f)); k=k+1u32 }
    var tracks=GridTracks { columns:cols.as_span(), rows:rows.as_span(), gap:Space.None }
    var g=b.grid(key("g"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)),tracks); b.child(root,g)
    var i=0u32
    while(i<n) { var c=b.row(key_of("c",i as u64),Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f))); b.child(g,c); b.grid_cell(c,(i%100u32) as u16,(i/100u32) as u16,1u16,1u16); i=i+1u32 }
    return b
}
func build_scroll(n : u32) : ViewBuilder {
    var b=new_view_builder(n+64u32,100.0f,100.0f); var root=view_root(&b)
    var sc=b.scroll(key("s"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(100.0f)),Axis.Vertical); b.child(root,sc)
    var col=b.column(key("col"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(n as float))); b.child(sc,col)
    var i=0u32
    while(i<n) { var c=b.row(key_of("r",i as u64),Style.new().width(Dim.Px(1.0f)).height(Dim.Px(1.0f))); b.child(col,c); i=i+1u32 }
    return b
}
func time_v2(b:&ViewBuilder,ctx:&LayoutContext,theme:&Theme,cap:u32) : i64 { var st=new_layout_state(cap); var s=std::chrono::Instant.now(); var _=layout_run(&mut st,b,ctx,theme); return s.elapsed().as_micros() }
func sorted5v(a:*mut i64) { var i=0; while(i<5) { var j=i+1; while(j<5) { if(unsafe(a[j])<unsafe(a[i])) { var t=unsafe(a[i]); unsafe{a[i]=unsafe(a[j]);a[j]=t} }; j=j+1 }; i=i+1 } }
func is_llvm_v2() : bool { var e=environment::get("CHEMICAL_UI_TEST_BACKEND"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("llvm",4u)}; return false }
func is_release_v2() : bool { var e=environment::get("CHEMICAL_UI_RELEASE"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("1",1u)}; return false }
func perf_v2() : bool {
    var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100000.0f,100000.0f)
    var wrap=build_wrap(10000u32); var wt:[5]i64; var i=0; while(i<5){wt[i]=time_v2(&wrap,&ctx,&theme,10064u32);i=i+1} sorted5v(&raw mut wt[0]); unsafe{printf("layout_v2 wrap_us p50=%ld p95=%ld\n",wt[2],wt[4])}
    var grid=build_grid(10000u32); var gt:[5]i64; i=0; while(i<5){gt[i]=time_v2(&grid,&ctx,&theme,10064u32);i=i+1} sorted5v(&raw mut gt[0]); unsafe{printf("layout_v2 grid_us p50=%ld p95=%ld\n",gt[2],gt[4])}
    var shell=build_scroll(100000u32); var stt:[5]i64; i=0; while(i<5){stt[i]=time_v2(&shell,&ctx,&theme,100064u32);i=i+1} sorted5v(&raw mut stt[0]); unsafe{printf("layout_v2 scroll100k_us p50=%ld p95=%ld\n",stt[2],stt[4])}
    var small=build_scroll(1000u32); var sctx=new_layout_context(empty_text_context(),100.0f,100.0f); var sm=new_layout_state(1064u32); var _=layout_run(&mut sm,&small,&sctx,&theme)
    var sm2=new_layout_state(1064u32); var s1=std::chrono::Instant.now(); var _s=layout_run(&mut sm2,&small,&sctx,&theme); var scroll1k=s1.elapsed().as_micros(); unsafe{printf("layout_v2 scroll1k_us=%ld\n",scroll1k)}
    var scroll_key=key("s"); var start=std::chrono::Instant.now(); layout_set_scroll(&mut sm,scroll_key,Point{x:0.0f,y:10.0f}); var _u=layout_run(&mut sm,&small,&sctx,&theme); var upd=start.elapsed().as_micros(); unsafe{printf("layout_v2 scroll_update_us=%ld\n",upd)}
    if(is_llvm_v2()) {
        if(wt[2] > 8000i64 || wt[4] > 12000i64) { return fail("wrap-gate") }
        if(gt[2] > 7000i64 || gt[4] > 11000i64) { return fail("grid-gate") }
        if(stt[2] > 80000i64 || stt[4] > 100000i64) { return fail("scroll-gate") }
        if(upd > 500i64) { return fail("scroll-update-gate") }
        if(scroll1k > 1000i64) { return fail("scroll-1k-gate") }
    }
    return true
}
struct RngV2 { var state : u64
    func next(&mut self) : u64 { var x=self.state; x=x^(x<<13u64); x=x^(x>>7u64); x=x^(x<<17u64); self.state=x; return x }
}
func property_wrap() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    var rng=RngV2{state:0x9E3779B97F4A7C15u64}; var iter=0u32
    while(iter<1000u32) {
        reset_view_builder(&mut b); var root=view_root(&b)
        var w=b.wrap(key("w"),Style.new().width(Dim.Px(100.0f)).height(Dim.Px(200.0f)).wrap(WrapMode.Wrap)); b.child(root,w)
        var n=(rng.next()%4u64+2u64) as u32; var widths=vector<float>(); var ids=vector<NodeId>()
        var k=0u32; while(k<n) { var cw=(rng.next()%25u64+10u64) as float; widths.push(cw); var c=box(&mut b,key_of("c",k as u64),cw,10.0f,w); ids.push(c); k=k+1u32 }
        var _=layout_run(&mut st,&b,&ctx,&theme)
        var x=0.0f; var y=0.0f; var lh=0.0f
        k=0u32
        while(k<n) {
            var cw=unsafe(widths.get_ptr(k)[0])
            if(x>0.0f && x+cw>100.0f) { x=0.0f; y=y+lh; lh=0.0f }
            var rr=get_rect(&st,unsafe(ids.get_ptr(k)[0]))
            if(!close(rr.x,x) || !close(rr.y,y) || !close(rr.w,cw)) { return fail("property-wrap") }
            x=x+cw; if(lh<10.0f) { lh=10.0f }
            k=k+1u32
        }
        iter=iter+1u32
    }
    return true
}
func property_grid() : bool {
    var b=new_view_builder(64u32,100.0f,100.0f); var theme=Theme.light(); var ctx=new_layout_context(empty_text_context(),100.0f,100.0f); var st=new_layout_state(64u32)
    var cols=vector<GridTrack>(); cols.push(GridTrack.Px(30.0f)); cols.push(GridTrack.Px(40.0f))
    var rows=vector<GridTrack>(); rows.push(GridTrack.Px(20.0f)); rows.push(GridTrack.Px(30.0f))
    var tracks=GridTracks{columns:cols.as_span(),rows:rows.as_span(),gap:Space.None}
    var iter=0u32
    while(iter<1000u32) {
        reset_view_builder(&mut b); var root=view_root(&b)
        var g=b.grid(key("g"),Style.new().width(Dim.Px(70.0f)).height(Dim.Px(50.0f)),tracks); b.child(root,g)
        var a=box(&mut b,key("a"),1.0f,1.0f,g); b.grid_cell(a,1u16,1u16,1u16,1u16)
        var _=layout_run(&mut st,&b,&ctx,&theme)
        if(!close(get_rect(&st,a).x,30.0f) || !close(get_rect(&st,a).y,20.0f) || !close(get_rect(&st,a).w,40.0f) || !close(get_rect(&st,a).h,30.0f)) { return fail("property-grid") }
        iter=iter+1u32
    }
    return true
}
public func run_layout_v2() : bool {
    if(!wrap_case()) { return false }
    if(!grid_case()) { return false }
    if(!grid_auto_span()) { return false }
    if(!scroll_case()) { return false }
    if(!scroll_axes_case()) { return false }
    if(!nested_scroll_case()) { return false }
    if(!layers_case()) { return false }
    if(!property_wrap()) { return false }
    if(!property_grid()) { return false }
    if(is_release_v2()) { if(!perf_v2()) { return false } }
    return true
}
}
