// P1.9 CPU rasterizer tests: deterministic torture PNGs, fuzz, clipping/seams.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using ui_internal::DisplayState;
using ui_internal::DisplayList;
using ui_internal::new_painter;
using ui_internal::ColorBuffer;
using ui_internal::Rasterizer;
using ui_internal::RasterStats;
using ui_internal::raster_render;
using ui_internal::raster_save_png;
using ui_internal::raster_pixel;
using ui_internal::raster_fill;
using ui_internal::raster_copy;

public namespace raster_tests {
func close(a : float, b : float) : bool { var d=a-b; if(d<0.0f){d=0.0f-d}; return d <= 0.5f }
func fail(name : *char) : bool { unsafe { printf("raster FAIL %s\n", name) }; return false }
func same_file(a : &path::Path, b : &path::Path) : bool {
    var ra=fs::read(a); var rb=fs::read(b)
    if(ra is Result.Err || rb is Result.Err) { return false }
    var Ok(da)=ra else return false
    var Ok(db)=rb else return false
    if(da.size() != db.size()) { return false }
    var i : size_t = 0u
    while(i < da.size()) { if(unsafe(da.get_ptr(i)[0]) != unsafe(db.get_ptr(i)[0])) { return false }; i=i+1u }
    return true
}

func sr(x : float, y : float, w : float, h : float, s : float) : Rect {
    return Rect.new(x*s,y*s,w*s,h*s)
}
func torture(scale : u32) : DisplayState {
    var w=320u32*scale; var h=240u32*scale
    var s=scale as float
    var ds=DisplayState.new(128u32); var theme=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(w as float,h as float))
    p.rect(sr(0.0f,0.0f,320.0f,240.0f,s),Color.rgba(24u8,32u8,48u8,255u8))
    p.rect(sr(12.0f,12.0f,120.0f,70.0f,s),Color.rgba(220u8,60u8,70u8,255u8))
    p.rounded_rect(sr(150.0f,18.0f,140.0f,80.0f,s),16.0f*s,Color.rgba(40u8,120u8,220u8,190u8))
    p.border(sr(300.0f,100.0f,80.0f,60.0f,s),8.0f*s,2.0f*s,Color.rgba(240u8,240u8,240u8,255u8))
    p.rect(sr(20.0f,100.0f,200.0f,100.0f,s),Color.rgba(80u8,200u8,120u8,120u8))
    {
        var g=p.push_clip(sr(30.0f,110.0f,120.0f,70.0f,s),0.0f)
        p.rect(sr(0.0f,0.0f,300.0f,200.0f,s),Color.rgba(255u8,200u8,40u8,180u8))
    }
    {
        var g=p.push_opacity(0.5f)
        p.rounded_rect(sr(180.0f,120.0f,110.0f,70.0f,s),24.0f*s,Color.rgba(255u8,255u8,255u8,255u8))
    }
    {
        var g=p.push_transform(Transform2D.translate(8.0f*s,4.0f*s))
        p.rect(sr(40.0f,190.0f,60.0f,24.0f,s),Color.rgba(240u8,80u8,220u8,200u8))
    }
    return ds
}

func render_torture(scale : u32, update : bool) : bool {
    var ds=torture(scale)
    var w=320u32*scale; var h=240u32*scale
    var buf=ColorBuffer.new(w,h)
    var r=Rasterizer.new(&raw mut buf)
    var st=r.render(&ds.list)
    if(st.drawn == 0u32 || st.pixels == 0u64) { return fail("torture-draw") }
    var full=string("lang/tests/ui/fixtures/raster/torture_")
    full.append_integer((w) as int); full.append('x'); full.append_integer(h as int); full.append_view(".png")
    var p=path::PathBuf.from(std::str.make_view(full.to_view()))
    if(update) {
        var sv=raster_save_png(&buf,&p.as_path())
        if(sv is Result.Err) { return fail("torture-save") }
    } else {
        var tmp=path::PathBuf.from("/tmp/raster_torture.png")
        var sv=raster_save_png(&buf,&tmp.as_path())
        if(sv is Result.Err) { return fail("torture-save-temp") }
        if(!same_file(&tmp.as_path(),&p.as_path())) { return fail("torture-golden") }
    }
    return true
}
func fuzz() : bool {
    var buf=ColorBuffer.new(180u32,140u32); var rng=Rng{state:0x12345678u64}
    var i=0u32
    while(i<500u32) {
        var ds=DisplayState.new(32u32); var theme=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(180.0f,140.0f))
        var x=(rng.next()%150u64) as float; var y=(rng.next()%110u64) as float; var w=(rng.next()%80u64+1u64) as float; var h=(rng.next()%60u64+1u64) as float
        var r=(rng.next()%255u64) as u8; var g=(rng.next()%255u64) as u8; var b=(rng.next()%255u64) as u8; var a=(rng.next()%255u64) as u8
        p.rounded_rect(Rect.new(x,y,w,h),(rng.next()%20u64) as float,Color.rgba(r,g,b,a))
        var back=Rasterizer.new(&raw mut buf); var _=back.render(&ds.list)
        i=i+1u32
    }
    return true
}
func same_buffer(a : &ColorBuffer, b : &ColorBuffer) : bool {
    if(a.width != b.width || a.height != b.height) { return false }
    var y=0u32
    while(y<a.height){var x=0u32; while(x<a.width){var ca=raster_pixel(a,x,y); var cb=raster_pixel(b,x,y); if(ca.r!=cb.r||ca.g!=cb.g||ca.b!=cb.b||ca.a!=cb.a){return false}; x=x+1u32}; y=y+1u32}
    return true
}
func simd_scalar() : bool {
    var ds=torture(1u32)
    var a=ColorBuffer.new(320u32,240u32)
    var b=ColorBuffer.new(320u32,240u32)
    var ra=Rasterizer.new(&raw mut a); var sa=ra.render(&ds.list)
    var rb=Rasterizer.new_scalar(&raw mut b); var sb=rb.render(&ds.list)
    if(sa.drawn != sb.drawn || sa.skipped != sb.skipped) { return fail("simd-stats") }
    if(!same_buffer(&a,&b)) { return fail("simd-scalar") }
    return true
}
func tile_seams() : bool {
    var ds=DisplayState.new(16u32); var theme=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(32.0f,16.0f))
    p.rounded_rect(Rect.new(4.0f,2.0f,24.0f,12.0f),4.0f,Color.rgba(30u8,180u8,210u8,255u8))
    p.border(Rect.new(12.0f,4.0f,16.0f,8.0f),3.0f,2.0f,Color.rgba(255u8,255u8,255u8,200u8))
    var full=ColorBuffer.new(32u32,16u32)
    var rf=Rasterizer.new(&raw mut full); var fs=rf.render(&ds.list)
    var tiled=ColorBuffer.new(32u32,16u32)
    var rt=Rasterizer.new(&raw mut tiled)
    var ts1=rt.render_tile(&ds.list,Rect.new(0.0f,0.0f,16.0f,16.0f),true)
    var ts2=rt.render_tile(&ds.list,Rect.new(16.0f,0.0f,16.0f,16.0f),false)
    if(fs.drawn == 0u32 || ts1.drawn == 0u32 || ts2.drawn == 0u32) { return fail("tile-stats") }
    if(!same_buffer(&full,&tiled)) { return fail("tile-seam") }
    return true
}
func edge_clamps() : bool {
    var buf=ColorBuffer.new(24u32,24u32)
    var ds=DisplayState.new(16u32); var theme=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(24.0f,24.0f))
    var z=0.0f
    var nan=z/z
    p.rect(Rect.new(-8.0f,-8.0f,-10.0f,-10.0f),Color.rgba(255u8,0u8,0u8,255u8))
    {
        var g=p.push_opacity(nan)
        p.rect(Rect.new(2.0f,2.0f,10.0f,10.0f),Color.rgba(0u8,255u8,0u8,255u8))
    }
    {
        var g=p.push_transform(Transform2D.translate(-1000.0f,0.0f))
        p.rect(Rect.new(0.0f,0.0f,10.0f,10.0f),Color.rgba(0u8,0u8,255u8,255u8))
    }
    var r=Rasterizer.new(&raw mut buf); var st=r.render(&ds.list)
    if(st.commands == 0u32) { return fail("edge-commands") }
    // The invalid/off-surface commands must not write outside the buffer; the
    // in-bounds NaN-opacity command must leave the cleared pixel untouched.
    var a=raster_pixel(&buf,5u32,5u32)
    if(a.r != 0u8 || a.g != 0u8 || a.b != 0u8) { return fail("edge-nan") }
    return true
}
func clip_seam() : bool {
    var buf=ColorBuffer.new(32u32,16u32)
    var ds=DisplayState.new(8u32); var theme=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(32.0f,16.0f))
    var g=p.push_clip(Rect.new(0.0f,0.0f,16.0f,16.0f),0.0f)
    p.rect(Rect.new(0.0f,0.0f,16.0f,16.0f),Color.rgba(255u8,0u8,0u8,255u8))
    p.rect(Rect.new(16.0f,0.0f,16.0f,16.0f),Color.rgba(0u8,0u8,255u8,255u8))
    var r=Rasterizer.new(&raw mut buf); var _=r.render(&ds.list)
    var a=raster_pixel(&buf,15u32,8u32); var b=raster_pixel(&buf,16u32,8u32)
    if(a.r != 255u8 || a.b != 0u8 || b.b != 0u8) { unsafe { printf("clip px15=%u,%u,%u,%u px16=%u,%u,%u,%u\n",a.r,a.g,a.b,a.a,b.r,b.g,b.b,b.a) }; return fail("clip") }
    return true
}
struct Rng { var state:u64; func next(&mut self):u64 { var x=self.state; x=x^(x<<13u64); x=x^(x>>7u64); x=x^(x<<17u64); self.state=x; return x } }
func is_llvm() : bool { var e=environment::get("CHEMICAL_UI_TEST_BACKEND"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("llvm",4u)}; return false }
func is_release() : bool { var e=environment::get("CHEMICAL_UI_RELEASE"); if(e is Option.Some){var Some(v)=e else return false; return v.to_view().equals_with_len("1",1u)}; return false }
func sorted5(a:*mut i64) { var i=0; while(i<5){var j=i+1; while(j<5){if(unsafe(a[j])<unsafe(a[i])){var t=unsafe(a[i]);unsafe{a[i]=unsafe(a[j]);a[j]=t}};j=j+1};i=i+1} }
func make_quads(aa : bool) : DisplayState {
    var ds=DisplayState.new(10032u32); var theme=Theme.light(); var p=new_painter(&raw mut ds.list,&raw mut theme,Size.new(1920.0f,1080.0f))
    var n=0u32
    while(n<10000u32) {
        var gx=n%100u32; var gy=n/100u32
        var x=(gx as float)*19.2f+1.0f; var y=(gy as float)*10.8f+1.0f
        if(aa) { p.rounded_rect(Rect.new(x,y,15.0f,7.0f),1.5f,Color.rgba(200u8,80u8,40u8,180u8)) }
        else { p.rect(Rect.new(x,y,15.0f,7.0f),Color.rgba(200u8,80u8,40u8,255u8)) }
        n=n+1u32
    }
    return ds
}
func perf_raster() : bool {
    var solid=make_quads(false); var aa=make_quads(true)
    var buf=ColorBuffer.new(1920u32,1080u32)
    var st:[5]i64; var i=0
    while(i<5){var r=Rasterizer.new(&raw mut buf); var s=std::chrono::Instant.now(); var _=r.render(&solid.list); st[i]=s.elapsed().as_micros(); i=i+1} sorted5(&raw mut st[0])
    unsafe { printf("raster solid10k_us p50=%ld p95=%ld\n",st[2],st[4]) }
    var at:[5]i64; i=0
    while(i<5){var r=Rasterizer.new(&raw mut buf); var s=std::chrono::Instant.now(); var _=r.render(&aa.list); at[i]=s.elapsed().as_micros(); i=i+1} sorted5(&raw mut at[0])
    unsafe { printf("raster aa10k_us p50=%ld p95=%ld\n",at[2],at[4]) }
    var ct:[5]i64; i=0
    while(i<5){var s=std::chrono::Instant.now(); buf.clear(); var c=Color.rgba(10u8,20u8,30u8,255u8); raster_fill(&raw mut buf,c); ct[i]=s.elapsed().as_micros(); i=i+1} sorted5(&raw mut ct[0])
    unsafe { printf("raster clearfill1080p_us p50=%ld p95=%ld\n",ct[2],ct[4]) }
    var dt:[5]i64; i=0
    while(i<5){var s=std::chrono::Instant.now(); raster_copy(&raw mut buf,&buf,0u32,0u32,200u32,120u32,300u32,200u32); dt[i]=s.elapsed().as_micros(); i=i+1} sorted5(&raw mut dt[0])
    unsafe { printf("raster damage200x120_us p50=%ld p95=%ld\n",dt[2],dt[4]) }
    if(is_llvm()) {
        if(st[2] > 2000i64) { return fail("solid-gate") }
        if(at[2] > 6000i64) { return fail("aa-gate") }
        if(ct[2] > 1000i64) { return fail("clear-gate") }
        if(dt[2] > 1500i64) { return fail("damage-gate") }
    }
    return true
}
func run_raster() : bool {
    var update=false; var e=environment::get("CHEMICAL_UI_UPDATE_GOLDENS")
    if(e is Option.Some){var Some(v)=e else return false; update=v.to_view().equals_with_len("1",1u)}
    if(!render_torture(1u32,update)){return false}
    if(!render_torture(2u32,update)){return false}
    if(!fuzz()){return false}
    if(!simd_scalar()){return false}
    if(!clip_seam()){return false}
    if(!tile_seams()){return false}
    if(!edge_clamps()){return false}
    if(is_release()) { if(!perf_raster()){return false} }
    return true
}
}
