// P1.8 display list and Painter. Commands only; no rasterization or host.
using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using std::Unit;

@direct_init
public struct GradientStop { var offset : float; var color : Color }
public variant Gradient {
    Linear(start : Point, end : Point, stops : vector<GradientStop>)
    Radial(center : Point, radius : float, stops : vector<GradientStop>)
}
public variant PathCommand {
    MoveTo(p : Point) LineTo(p : Point) QuadTo(c : Point, p : Point)
    CubicTo(c1 : Point, c2 : Point, p : Point) Close()
}
@direct_init
public struct Path { var commands : vector<PathCommand>; var fill_rule : u8 }

@direct_init
public struct CanvasPaint {
    var node : NodeId
    var key : Key
    var id : CanvasId
    var bounds : Rect
    var scale : float
}

public namespace ui_internal {

public enum DisplayCommandKind {
    Rect, RoundedRect, Border, Line, Path, Gradient, Shadow, Image, Icon, Text,
    ClipPush, ClipPop, LayerPush, LayerPop, TransformPush, TransformPop
}

@direct_init
public struct DisplayCommand {
    var kind : DisplayCommandKind
    var rect : Rect
    var p0 : Point
    var p1 : Point
    var radius : float
    var width : float
    var opacity : float
    var color : Color
    var transform : Transform2D
    var clip : Rect
    var has_clip : bool
    var text : string_view
    var text_style : TextStyle
    var baseline : float
    var layer : u8
    var z : i32
    var path_index : u32
    var gradient_index : u32
    var image : ImageHandle
    var icon : IconId
}

@direct_init
public struct DisplayList {
    var commands : vector<DisplayCommand>
    var paths : vector<Path>
    var gradients : vector<Gradient>
    var viewport : Size
    var text_arena : FrameArena
}

@direct_init
public struct PaintProps {
    var background : Color
    var foreground : Color
    var border_width : float
    var border_color : Color
    var radius : float
    var shadow : Shadow
    var opacity : float
    var overflow : Overflow
    var z : i32
    var text : TextStyle
}
func display_empty_paint() : PaintProps {
    return PaintProps {
        background : Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        foreground : Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        border_width : 0.0f, border_color : Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        radius : 0.0f, shadow : Shadow { x : 0.0f, y : 0.0f, blur : 0.0f, spread : 0.0f, color : Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 } },
        opacity : 1.0f, overflow : Overflow.Visible, z : 0i32,
        text : layout_text_style_default()
    }
}
func display_resolve(st : &mut DisplayState, style_id : u32, theme : &Theme) : PaintProps {
    if(st.has_paint && st.last_style_id == style_id) { return st.last_paint }
    var r = style_resolve(Style { id : style_id }, theme)
    var p = PaintProps {
        background : r.background, foreground : r.foreground,
        border_width : r.border_width, border_color : r.border_color,
        radius : r.radius, shadow : r.shadow, opacity : r.opacity,
        overflow : r.overflow, z : r.z, text : r.text
    }
    st.last_style_id = style_id; st.last_paint = p; st.has_paint = true
    return p
}

@direct_init
public struct HitRecord {
    var node : NodeId
    var key : Key
    var rect : Rect
    var device_rect : Rect
    var transform : Transform2D
    var clip : Rect
    var device_clip : Rect
    var has_clip : bool
    var z : i32
    var order : u32
    var tab_index : i32
    var focusable : bool
    var interactive : bool
    var disabled : bool
    var scrollable : bool
    var parent : u32
}

@direct_init
public struct DisplayState {
    var list : DisplayList
    var last_style_id : u32
    var last_paint : PaintProps
    var has_paint : bool
    var focus_key : Option<Key>
    var hover_key : Option<Key>
    var hits : vector<HitRecord>
    public func new(capacity : u32) : DisplayState {
        var l = DisplayList { commands : vector<DisplayCommand>(), paths : vector<Path>(),
            gradients : vector<Gradient>(), viewport : Size.zero(),
            text_arena : FrameArena.new(1024u * 1024u) }
        l.commands.reserve(capacity as size_t)
        return DisplayState { list : l, last_style_id : 0u32,
            last_paint : display_empty_paint(), has_paint : false,
            focus_key : Option.None<Key>(), hover_key : Option.None<Key>(),
            hits : vector<HitRecord>() }
    }
}

public interface RenderBackend {
    func submit(&mut self, display : &DisplayList, damage : &[Rect]) : Result<Unit, UiError>
    func present(&mut self) : Result<Unit, UiError>
    func resize(&mut self, width : u32, height : u32)
    func device_lost(&mut self)
}

public struct RecordingBackend {
    var submits : u32
    var presents : u32
    var resizes : u32
    var lost : u32
    var last_commands : u32
    var last_damage : vector<Rect>
    @make func make() : RecordingBackend {
        return RecordingBackend { submits : 0u32, presents : 0u32, resizes : 0u32,
            lost : 0u32, last_commands : 0u32, last_damage : vector<Rect>() }
    }
}

impl RenderBackend for RecordingBackend {
    func submit(&mut self, display : &DisplayList, damage : &[Rect]) : Result<Unit, UiError> {
        self.submits = self.submits + 1u32
        self.last_commands = display.commands.size() as u32
        self.last_damage.clear()
        var i : size_t = 0u
        while(i < damage.size()) { self.last_damage.push(damage[i]); i = i + 1u }
        return Result.Ok<Unit, UiError>(Unit{})
    }
    func present(&mut self) : Result<Unit, UiError> {
        self.presents = self.presents + 1u32
        return Result.Ok<Unit, UiError>(Unit{})
    }
    func resize(&mut self, width : u32, height : u32) {
        self.resizes = self.resizes + 1u32
    }
    func device_lost(&mut self) {
        self.lost = self.lost + 1u32
    }
}

func display_command_bounds(c : DisplayCommand, viewport : Size) : Option<Rect> {
    var r = c.rect
    if(c.kind == DisplayCommandKind.Line) {
        r = Rect.from_ltrb(min32(c.p0.x,c.p1.x), min32(c.p0.y,c.p1.y), max32(c.p0.x,c.p1.x), max32(c.p0.y,c.p1.y))
    } else if(c.kind == DisplayCommandKind.Text) {
        if(r.w <= 0.0f) { r = Rect.new(r.x, r.y, 1.0f, max32(1.0f, c.baseline - r.y)) }
    } else if(c.kind == DisplayCommandKind.ClipPush || c.kind == DisplayCommandKind.ClipPop ||
              c.kind == DisplayCommandKind.LayerPush || c.kind == DisplayCommandKind.LayerPop ||
              c.kind == DisplayCommandKind.TransformPush || c.kind == DisplayCommandKind.TransformPop) {
        return Option.None<Rect>()
    }
    if(r.w <= 0.0f || r.h <= 0.0f) { return Option.None<Rect>() }
    var tr = display_transform_rect(c.transform, r)
    if(c.has_clip) { tr = display_rect_intersect(tr, c.clip, true) }
    tr = display_rect_intersect(tr, Rect.new(0.0f,0.0f,viewport.width,viewport.height), true)
    if(tr.w <= 0.0f || tr.h <= 0.0f) { return Option.None<Rect>() }
    return Option.Some<Rect>(tr)
}

public func display_damage(st : &DisplayState, viewport : Size, full : bool) : vector<Rect> {
    var out = vector<Rect>()
    if(full) { out.push(Rect.new(0.0f,0.0f,viewport.width,viewport.height)); return out }
    unsafe {
        var i : size_t = 0u
        while(i < st.list.commands.size()) {
            var b = display_command_bounds(st.list.commands.get_ptr(i)[0], viewport)
            if(b is Option.Some) { var Some(r)=b else continue; out.push(r) }
            i = i + 1u
        }
    }
    return out
}

func display_kind_name(k : DisplayCommandKind) : string_view {
    switch(k) {
        DisplayCommandKind.Rect => return "rect"
        DisplayCommandKind.RoundedRect => return "roundrect"
        DisplayCommandKind.Border => return "border"
        DisplayCommandKind.Line => return "line"
        DisplayCommandKind.Path => return "path"
        DisplayCommandKind.Gradient => return "gradient"
        DisplayCommandKind.Shadow => return "shadow"
        DisplayCommandKind.Image => return "image"
        DisplayCommandKind.Icon => return "icon"
        DisplayCommandKind.Text => return "text"
        DisplayCommandKind.ClipPush => return "clippush"
        DisplayCommandKind.ClipPop => return "clippop"
        DisplayCommandKind.LayerPush => return "layerpush"
        DisplayCommandKind.LayerPop => return "layerpop"
        DisplayCommandKind.TransformPush => return "transformpush"
        DisplayCommandKind.TransformPop => return "transformpop"
    }
    return "unknown"
}

public func display_serialize(st : &DisplayState) : string {
    unsafe {
    var out = string("DISPLAY 1\n")
    out.append_view("viewport "); out.append_float(st.list.viewport.width,3); out.append(' '); out.append_float(st.list.viewport.height,3); out.append('\n')
    out.append_view("commands "); out.append_integer(st.list.commands.size() as int); out.append('\n')
    out.append_view("paths "); out.append_integer(st.list.paths.size() as int); out.append_view(" gradients "); out.append_integer(st.list.gradients.size() as int); out.append('\n')
    var i : size_t = 0u
    while(i < st.list.commands.size()) {
        var c = st.list.commands.get_ptr(i)[0]
        out.append_integer(i as int); out.append(' '); out.append_view(display_kind_name(c.kind))
        out.append_view(" rect "); out.append_float(c.rect.x,3); out.append(' '); out.append_float(c.rect.y,3); out.append(' '); out.append_float(c.rect.w,3); out.append(' '); out.append_float(c.rect.h,3)
        out.append_view(" color "); out.append_integer(c.color.r as int); out.append(' '); out.append_integer(c.color.g as int); out.append(' '); out.append_integer(c.color.b as int); out.append(' '); out.append_integer(c.color.a as int)
        out.append_view(" radius "); out.append_float(c.radius,3); out.append_view(" width "); out.append_float(c.width,3)
        out.append_view(" opacity "); out.append_float(c.opacity,3); out.append_view(" layer "); out.append_integer(c.layer as int); out.append_view(" z "); out.append_integer(c.z as int)
        out.append_view(" transform "); var tm : size_t = 0u; while(tm < 6u) { out.append_float(c.transform.m[tm],3); out.append(' '); tm=tm+1u }
        out.append_view("clip "); if(c.has_clip) { out.append_float(c.clip.x,3); out.append(' '); out.append_float(c.clip.y,3); out.append(' '); out.append_float(c.clip.w,3); out.append(' '); out.append_float(c.clip.h,3) } else { out.append_view("none") }
        out.append_view(" text "); out.append_view(&c.text); out.append_view(" baseline "); out.append_float(c.baseline,3)
        out.append_view(" path "); out.append_integer(c.path_index as int); out.append_view(" gradient "); out.append_integer(c.gradient_index as int)
        out.append('\n')
        i = i + 1u
    }
    return out

    }}

func display_empty_command(kind : DisplayCommandKind) : DisplayCommand {
    return DisplayCommand {
        kind : kind, rect : Rect.zero(), p0 : Point.zero(), p1 : Point.zero(),
        radius : 0.0f, width : 0.0f, opacity : 1.0f,
        color : Color { r : 0u8, g : 0u8, b : 0u8, a : 0u8 },
        transform : Transform2D.identity(), clip : Rect.zero(), has_clip : false,
        text : "", text_style : layout_text_style_default(),
        baseline : 0.0f, layer : 0u8, z : 0i32, path_index : 0u32,
        gradient_index : 0u32, image : ImageHandle.zero(), icon : IconId.zero()
    }
}

func layout_text_style_default() : TextStyle {
    return TextStyle {
        family : FontFamily.Ui, family_name : "", size : 14.0f,
        weight : FontWeight.Regular, italic : false,
        line_height : 20.0f / 14.0f, letter_spacing : 0.0f,
        features : TextFeatures { ligatures : false, tabular_numbers : false,
            slashed_zero : false, stylistic_set : 0u8, raw_tags : zeroed<[0]u32>() },
        tab_size : 4u8
    }
}

func display_emit(list : *mut DisplayList, c : DisplayCommand) {
    unsafe {
        if(list == null) { return }
        if(c.text.size() > 0u) {
            var p = list.text_arena.alloc(c.text.size(), 1u)
            if(p is Result.Ok) {
                var Ok(dst) = p else return
                memcpy(dst as *mut void, c.text.data() as *mut void, c.text.size())
                c.text = string_view(dst as *char, c.text.size())
            }
        }
        list.commands.push(c)
    }
}

func display_rect_intersect(a : Rect, b : Rect, has_b : bool) : Rect {
    if(!has_b) { return a }
    var x0 = max32(a.x, b.x)
    var y0 = max32(a.y, b.y)
    var x1 = min32(a.x2(), b.x2())
    var y1 = min32(a.y2(), b.y2())
    if(x1 <= x0 || y1 <= y0) { return Rect.zero() }
    return Rect.from_ltrb(x0, y0, x1, y1)
}

func display_transform_rect(t : Transform2D, r : Rect) : Rect {
    var p0 = t.transform_point(Point { x : r.x, y : r.y })
    var p1 = t.transform_point(Point { x : r.x2(), y : r.y })
    var p2 = t.transform_point(Point { x : r.x, y : r.y2() })
    var p3 = t.transform_point(Point { x : r.x2(), y : r.y2() })
    var x0 = min32(min32(p0.x, p1.x), min32(p2.x, p3.x))
    var y0 = min32(min32(p0.y, p1.y), min32(p2.y, p3.y))
    var x1 = max32(max32(p0.x, p1.x), max32(p2.x, p3.x))
    var y1 = max32(max32(p0.y, p1.y), max32(p2.y, p3.y))
    return Rect.from_ltrb(x0, y0, x1, y1)
}

func display_clamp_opacity(v : float) : float {
    if(v != v || v < 0.0f) { return 0.0f }
    if(v > 1.0f) { return 1.0f }
    return v
}

func display_build_node(state : &mut DisplayState, b : &ViewBuilder, layout : &LayoutState,
                        theme : &Theme, textctx : &TextContext, index : u32,
                        parent_transform : Transform2D, parent_clip : Rect,
                        has_parent_clip : bool, overlay_root : bool) {
    if(index == 0u32 || index as size_t >= layout.result.nodes.size()) { return }
    var n = layout.result.nodes.get_ptr(index as size_t)
    unsafe {
        if(!n.id.is_valid()) { return }
        var tr = parent_transform
        var has_node_transform = false
        if((n.id.index() as size_t) < b.node_count_value + 1u32) {
            var vn = b.node_base + (n.id.index() - 1u32)
            if(vn.meta_index != VIEW_NONE && (vn.meta_index as size_t) < b.metas.size()) {
                var m = b.metas.get_ptr(vn.meta_index as size_t)
                if(m.has_transform) { tr = parent_transform.multiply(m.transform); has_node_transform = true }
            }
        }
        var clip = parent_clip
        var has_clip = has_parent_clip
        if(n.has_clip) {
            var c = n.clip
            if(has_clip) { c = display_rect_intersect(c, parent_clip, true) }
            clip = c; has_clip = true
        }
        if(n.spec.overflow == Overflow.Clip) {
            var c = n.rect
            if(has_clip) { c = display_rect_intersect(c, clip, true) }
            clip = c; has_clip = true
        }
        var visible = n.rect.is_finite() && n.rect.w > 0.0f && n.rect.h > 0.0f
        if(has_clip && !overlay_root) {
            var trr = display_transform_rect(tr, n.rect)
            if(!trr.contains(Point { x : clip.x, y : clip.y }) &&
               !trr.contains(Point { x : clip.x2(), y : clip.y2() }) &&
               !clip.contains(Point { x : trr.x, y : trr.y }) &&
               !clip.contains(Point { x : trr.x2(), y : trr.y2() })) { visible = false }
        }
        if(!visible) {
            // Overlay nodes still get a chance to escape a clipped ancestor.
            if(!n.is_overlay) { return }
        }
        var rs = display_resolve(state, n.style_id, theme)
        var opacity = display_clamp_opacity(rs.opacity)
        var meta_focusable = false
        var meta_interactive = false
        var meta_disabled = false
        var meta_tab_index = 0i32
        var meta_has_command = false
        if((n.id.index() as size_t) < b.node_count_value + 1u32) {
            var vn0 = b.node_base + (n.id.index() - 1u32)
            if(vn0.meta_index != VIEW_NONE && (vn0.meta_index as size_t) < b.metas.size()) {
                var m0 = b.metas.get_ptr(vn0.meta_index as size_t)
                meta_focusable = m0.focusable
                meta_has_command = m0.has_command
                meta_tab_index = m0.tab_index
                meta_disabled = m0.disabled
                if(m0.has_semantics) { meta_disabled = meta_disabled || m0.semantics.states.disabled }
            }
        }
        meta_interactive = meta_focusable || meta_has_command
        if(meta_focusable || meta_has_command || n.kind == ViewKind.Scroll) {
            var dev_rect = display_transform_rect(tr, n.rect)
            var dev_clip = Rect.zero()
            if(has_clip) { dev_clip = display_transform_rect(tr, clip) }
            state.hits.push(HitRecord {
                node : n.id, key : n.key, rect : n.rect, device_rect : dev_rect,
                transform : tr, clip : clip, device_clip : dev_clip, has_clip : has_clip,
                z : n.z_index, order : state.hits.size() as u32, tab_index : meta_tab_index,
                focusable : meta_focusable, interactive : meta_interactive,
                disabled : meta_disabled, scrollable : n.kind == ViewKind.Scroll,
                parent : n.parent
            })
        }
        if(has_node_transform) {
            var cptr = display_empty_command(DisplayCommandKind.TransformPush)
            cptr.transform = tr; cptr.opacity = opacity; cptr.clip = clip; cptr.has_clip = has_clip
            display_emit(&raw mut state.list, cptr)
        }
        if(has_clip) {
            var cp = display_empty_command(DisplayCommandKind.ClipPush)
            cp.clip = clip; cp.has_clip = true; cp.transform = tr
            display_emit(&raw mut state.list, cp)
        }
        if(opacity < 1.0f) {
            var lp = display_empty_command(DisplayCommandKind.LayerPush)
            lp.opacity = opacity; lp.rect = n.rect; lp.transform = tr
            display_emit(&raw mut state.list, lp)
        }
        if(n.is_overlay) {
            var op = display_empty_command(DisplayCommandKind.LayerPush)
            op.layer = n.overlay_layer as u8; op.z = n.z_index; op.rect = n.rect; op.transform = tr
            display_emit(&raw mut state.list, op)
        }
        // Node paint primitives.
        if(rs.shadow.blur > 0.0f || rs.shadow.spread > 0.0f || rs.shadow.color.a > 0u8) {
            var sc = display_empty_command(DisplayCommandKind.Shadow)
            sc.rect = n.rect; sc.radius = rs.radius; sc.color = rs.shadow.color
            sc.transform = tr; sc.clip = clip; sc.has_clip = has_clip
            sc.p0 = Point { x : rs.shadow.x, y : rs.shadow.y }
            sc.p1 = Point { x : rs.shadow.blur, y : rs.shadow.spread }
            display_emit(&raw mut state.list, sc)
        }
        if(rs.background.a > 0u8) {
            var bk = DisplayCommandKind.Rect
            if(rs.radius > 0.0f) { bk = DisplayCommandKind.RoundedRect }
            var bc = display_empty_command(bk)
            bc.rect = n.rect; bc.radius = rs.radius; bc.color = rs.background
            bc.transform = tr; bc.clip = clip; bc.has_clip = has_clip
            display_emit(&raw mut state.list, bc)
        }
        if(rs.border_width > 0.0f && rs.border_color.a > 0u8) {
            var bo = display_empty_command(DisplayCommandKind.Border)
            bo.rect = n.rect; bo.radius = rs.radius; bo.width = rs.border_width
            bo.color = rs.border_color; bo.transform = tr; bo.clip = clip; bo.has_clip = has_clip
            display_emit(&raw mut state.list, bo)
        }
        if(layout_is_text(n.kind)) {
            var tc = TextConstraints { max_width : 0.0f, max_lines : 0u16,
                wrap : WrapMode.NoWrap, overflow : TextOverflow.Clip,
                tab_size : rs.text.tab_size, direction : Direction.Ltr }
            var tm = textctx.measure(n.text, rs.text, tc)
            var tc_cmd = display_empty_command(DisplayCommandKind.Text)
            tc_cmd.rect = Rect.new(n.rect.x, n.rect.y, tm.width, tm.height)
            tc_cmd.text = n.text; tc_cmd.text_style = rs.text
            tc_cmd.color = rs.foreground; tc_cmd.baseline = n.baseline
            tc_cmd.transform = tr; tc_cmd.clip = clip; tc_cmd.has_clip = has_clip
            display_emit(&raw mut state.list, tc_cmd)
        } else if(n.kind == ViewKind.Image || n.kind == ViewKind.Icon) {
            var ik = DisplayCommandKind.Image
            if(n.kind == ViewKind.Icon) { ik = DisplayCommandKind.Icon }
            var ic = display_empty_command(ik)
            ic.rect = n.rect; ic.transform = tr; ic.clip = clip; ic.has_clip = has_clip
            if((n.id.index() as size_t) < b.node_count_value + 1u32) {
                var vn = b.node_base + (n.id.index() - 1u32)
                if(vn.meta_index != VIEW_NONE && (vn.meta_index as size_t) < b.metas.size()) {
                    var m = b.metas.get_ptr(vn.meta_index as size_t)
                    ic.icon = m.icon
                }
            }
            display_emit(&raw mut state.list, ic)
        }
        // Normal children first.
        var cur = n.first_child
        while(cur != 0u32) {
            var cp = layout.result.nodes.get_ptr(cur as size_t)
            if(cp.kind != ViewKind.Overlay) { display_build_node(state, b, layout, theme, textctx, cur, tr, clip, has_clip, overlay_root) }
            cur = cp.next_sibling
        }
        // Overlay children after normal flow, by layer bucket then tree order.
        var layer = 0u32
        while(layer < 5u32) {
            cur = n.first_child
            while(cur != 0u32) {
                var cp = layout.result.nodes.get_ptr(cur as size_t)
                if(cp.kind == ViewKind.Overlay && (cp.overlay_layer as u32) == layer) {
                    display_build_node(state, b, layout, theme, textctx, cur, tr, Rect.zero(), false, true)
                }
                cur = cp.next_sibling
            }
            layer = layer + 1u32
        }
        var has_ring_state = state.focus_key.is_some() || state.hover_key.is_some()
        if(has_ring_state) {
        var is_focus = false
        switch(state.focus_key) {
            Some(fk) => { is_focus = fk.equals_key(n.key) }
            None() => { }
        }
        var is_hover = false
        switch(state.hover_key) {
            Some(hk) => { is_hover = hk.equals_key(n.key) }
            None() => { }
        }
        if((is_focus || is_hover) && n.rect.w > 0.0f && n.rect.h > 0.0f) {
            var fr = Rect.from_ltrb(n.rect.x - 2.0f, n.rect.y - 2.0f,
                n.rect.x2() + 2.0f, n.rect.y2() + 2.0f)
            var token = ColorToken.BorderStrong
            var fw = 1.0f
            if(is_focus) { token = ColorToken.FocusRing; fw = 2.0f }
            var frc = theme.color(token)
            var fcmd = display_empty_command(DisplayCommandKind.Border)
            fcmd.rect = fr; fcmd.radius = rs.radius; fcmd.width = fw
            fcmd.color = frc; fcmd.transform = tr; fcmd.clip = clip; fcmd.has_clip = has_clip
            display_emit(&raw mut state.list, fcmd)
        }
        }
        if(n.is_overlay) {
            var op = display_empty_command(DisplayCommandKind.LayerPop)
            op.layer = n.overlay_layer as u8; op.z = n.z_index
            display_emit(&raw mut state.list, op)
        }
        if(opacity < 1.0f) {
            display_emit(&raw mut state.list, display_empty_command(DisplayCommandKind.LayerPop))
        }
        if(has_clip) {
            display_emit(&raw mut state.list, display_empty_command(DisplayCommandKind.ClipPop))
        }
        if(has_node_transform) {
            display_emit(&raw mut state.list, display_empty_command(DisplayCommandKind.TransformPop))
        }
    }
}

public func display_build(st : &mut DisplayState, b : &ViewBuilder, layout : &LayoutState,
                          theme : &Theme, textctx : &TextContext, viewport : Size) : u32 {
    st.list.commands.clear()
    st.list.paths.clear()
    st.list.gradients.clear()
    st.list.text_arena.reset()
    st.hits.clear()
    st.list.viewport = viewport
    display_build_node(st, b, layout, theme, textctx, layout.result.root_index,
        Transform2D.identity(), Rect.new(0.0f,0.0f,viewport.width,viewport.height), true, false)
    return st.list.commands.size() as u32
}

// ── Painter: records into a DisplayList; never rasterizes ──────────────────
func display_emit_color(list : *mut DisplayList, kind : DisplayCommandKind, r : Rect,
                        color : Color, radius : float, width : float,
                        tr : Transform2D, clip : Rect, has_clip : bool, opacity : float,
                        p0 : Point, p1 : Point) {
    var c = display_empty_command(kind)
    c.rect = r; c.color = color; c.radius = radius; c.width = width
    c.transform = tr; c.clip = clip; c.has_clip = has_clip; c.opacity = opacity
    c.p0 = p0; c.p1 = p1
    display_emit(list, c)
}

} // namespace ui_internal

public struct Painter {
    internal var list : *mut ui_internal::DisplayList
    internal var theme_ptr : *Theme
    internal var size_value : Size
    internal var transform : Transform2D
    internal var clip : Rect
    internal var has_clip : bool
    internal var opacity : float
    internal var clip_stack : [16]Rect
    internal var transform_stack : [16]Transform2D
    internal var opacity_stack : [16]float
    internal var clip_count : u32
    internal var transform_count : u32
    internal var opacity_count : u32

    public func size(&self) : Size { return self.size_value }
    public func theme(&self) : &Theme { unsafe { return &*self.theme_ptr } }
    public func color(&self, t : ColorToken) : Color { unsafe { return self.theme_ptr.color(t) } }

    public func push_clip(&mut self, r : Rect, radius : float) : ClipGuard {
        if(self.clip_count >= 16u32) { return ClipGuard { owner : null, active : false } }
        self.clip_stack[self.clip_count as size_t] = self.clip
        self.clip_count = self.clip_count + 1u32
        if(self.has_clip) { r = ui_internal::display_rect_intersect(r, self.clip, true) }
        self.clip = r; self.has_clip = true
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.ClipPush)
        c.rect = r; c.radius = radius; c.clip = r; c.has_clip = true; c.transform = self.transform; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
        return ClipGuard { owner : unsafe(&raw mut self), active : true }
    }
    public func push_opacity(&mut self, o : float) : LayerGuard {
        if(self.opacity_count >= 16u32) { return LayerGuard { owner : null, active : false } }
        self.opacity_stack[self.opacity_count as size_t] = self.opacity
        self.opacity_count = self.opacity_count + 1u32
        self.opacity = ui_internal::display_clamp_opacity(self.opacity * o)
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.LayerPush)
        c.opacity = self.opacity; c.rect = Rect.new(0.0f,0.0f,self.size_value.width,self.size_value.height); c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip
        ui_internal::display_emit(self.list, c)
        return LayerGuard { owner : unsafe(&raw mut self), active : true }
    }
    public func push_transform(&mut self, t : Transform2D) : TransformGuard {
        if(self.transform_count >= 16u32) { return TransformGuard { owner : null, active : false } }
        self.transform_stack[self.transform_count as size_t] = self.transform
        self.transform_count = self.transform_count + 1u32
        self.transform = self.transform.multiply(t)
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.TransformPush)
        c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
        return TransformGuard { owner : unsafe(&raw mut self), active : true }
    }
    public func rect(&mut self, r : Rect, color : Color) {
        ui_internal::display_emit_color(self.list, ui_internal::DisplayCommandKind.Rect, r, color, 0.0f, 0.0f, self.transform, self.clip, self.has_clip, self.opacity, Point.zero(), Point.zero())
    }
    public func rounded_rect(&mut self, r : Rect, radius : float, color : Color) {
        ui_internal::display_emit_color(self.list, ui_internal::DisplayCommandKind.RoundedRect, r, color, radius, 0.0f, self.transform, self.clip, self.has_clip, self.opacity, Point.zero(), Point.zero())
    }
    public func border(&mut self, r : Rect, radius : float, width : float, color : Color) {
        ui_internal::display_emit_color(self.list, ui_internal::DisplayCommandKind.Border, r, color, radius, width, self.transform, self.clip, self.has_clip, self.opacity, Point.zero(), Point.zero())
    }
    public func line(&mut self, a : Point, b : Point, width : float, color : Color) {
        ui_internal::display_emit_color(self.list, ui_internal::DisplayCommandKind.Line, Rect.zero(), color, 0.0f, width, self.transform, self.clip, self.has_clip, self.opacity, a, b)
    }
    public func path(&mut self, path : &Path, color : Color) {
        var idx : u32 = 0u32
        unsafe { idx = self.list.paths.size() as u32 }
        var p = Path { commands : vector<PathCommand>(), fill_rule : path.fill_rule }
        var pi : size_t = 0u
        while(pi < path.commands.size()) { p.commands.push(unsafe(path.commands.get_ptr(pi)[0])); pi = pi + 1u }
        unsafe { self.list.paths.push(p) }
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.Path)
        c.color = color; c.path_index = idx; c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
    }
    public func gradient_rect(&mut self, r : Rect, g : Gradient) {
        var idx : u32 = 0u32
        unsafe { idx = self.list.gradients.size() as u32; self.list.gradients.push(g) }
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.Gradient)
        c.rect = r; c.gradient_index = idx; c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
    }
    public func shadow(&mut self, r : Rect, radius : float, s : Shadow) {
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.Shadow)
        c.rect = r; c.radius = radius; c.color = s.color; c.p0 = Point { x : s.x, y : s.y }; c.p1 = Point { x : s.blur, y : s.spread }
        c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
    }
    public func image(&mut self, img : ImageHandle, r : Rect, fit : ImageFit) {
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.Image)
        c.rect = r; c.image = img; c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
    }
    public func text(&mut self, text : string_view, pos : Point, style : TextStyle, color : Color) {
        var c = ui_internal::display_empty_command(ui_internal::DisplayCommandKind.Text)
        c.p0 = pos; c.text = text; c.text_style = style; c.color = color
        c.transform = self.transform; c.clip = self.clip; c.has_clip = self.has_clip; c.opacity = self.opacity
        ui_internal::display_emit(self.list, c)
    }

    internal func pop_clip(&mut self) {
        if(self.clip_count == 0u32) { return }
        self.clip_count = self.clip_count - 1u32
        self.clip = self.clip_stack[self.clip_count as size_t]
        if(self.clip_count == 0u32) { self.has_clip = false }
        ui_internal::display_emit(self.list, ui_internal::display_empty_command(ui_internal::DisplayCommandKind.ClipPop))
    }
    internal func pop_opacity(&mut self) {
        if(self.opacity_count == 0u32) { return }
        self.opacity_count = self.opacity_count - 1u32
        self.opacity = self.opacity_stack[self.opacity_count as size_t]
        ui_internal::display_emit(self.list, ui_internal::display_empty_command(ui_internal::DisplayCommandKind.LayerPop))
    }
    internal func pop_transform(&mut self) {
        if(self.transform_count == 0u32) { return }
        self.transform_count = self.transform_count - 1u32
        self.transform = self.transform_stack[self.transform_count as size_t]
        ui_internal::display_emit(self.list, ui_internal::display_empty_command(ui_internal::DisplayCommandKind.TransformPop))
    }
}

public struct ClipGuard {
    internal var owner : *mut Painter
    internal var active : bool
    @delete func delete(&mut self) { if(self.active && self.owner != null) { unsafe((*self.owner).pop_clip()) } }
}
public struct LayerGuard {
    internal var owner : *mut Painter
    internal var active : bool
    @delete func delete(&mut self) { if(self.active && self.owner != null) { unsafe((*self.owner).pop_opacity()) } }
}
public struct TransformGuard {
    internal var owner : *mut Painter
    internal var active : bool
    @delete func delete(&mut self) { if(self.active && self.owner != null) { unsafe((*self.owner).pop_transform()) } }
}

public namespace ui_internal {
public func new_painter(list : *mut DisplayList, theme : *Theme, size : Size) : Painter {
    return Painter { list : list, theme_ptr : theme, size_value : size,
        transform : Transform2D.identity(), clip : Rect.new(0.0f,0.0f,size.width,size.height),
        has_clip : true, opacity : 1.0f, clip_stack : zeroed<[16]Rect>(),
        transform_stack : zeroed<[16]Transform2D>(), opacity_stack : zeroed<[16]float>(),
        clip_count : 0u32, transform_count : 0u32, opacity_count : 0u32 }
}
public func display_command_count(st : &DisplayState) : u32 { return st.list.commands.size() as u32 }
public func display_kind(st : &DisplayState, i : u32) : DisplayCommandKind {
    if(i as size_t >= st.list.commands.size()) { return DisplayCommandKind.Rect }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].kind)
}
public func display_rect(st : &DisplayState, i : u32) : Rect {
    if(i as size_t >= st.list.commands.size()) { return Rect.zero() }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].rect)
}
public func display_text(st : &DisplayState, i : u32) : Option<string_view> {
    if(i as size_t >= st.list.commands.size()) { return Option.None<string_view>() }
    return Option.Some<string_view>(unsafe(st.list.commands.get_ptr(i as size_t)[0].text))
}
public func display_transform(st : &DisplayState, i : u32) : Transform2D {
    if(i as size_t >= st.list.commands.size()) { return Transform2D.identity() }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].transform)
}
public func display_clip(st : &DisplayState, i : u32) : Option<Rect> {
    if(i as size_t >= st.list.commands.size()) { return Option.None<Rect>() }
    var c = st.list.commands.get_ptr(i as size_t)
    if(!unsafe(c.has_clip)) { return Option.None<Rect>() }
    return Option.Some<Rect>(unsafe(c.clip))
}
public func display_opacity(st : &DisplayState, i : u32) : float {
    if(i as size_t >= st.list.commands.size()) { return 0.0f }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].opacity)
}
public func display_layer(st : &DisplayState, i : u32) : u8 {
    if(i as size_t >= st.list.commands.size()) { return 0u8 }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].layer)
}
public func display_z(st : &DisplayState, i : u32) : i32 {
    if(i as size_t >= st.list.commands.size()) { return 0i32 }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].z)
}
public func display_baseline(st : &DisplayState, i : u32) : float {
    if(i as size_t >= st.list.commands.size()) { return 0.0f }
    return unsafe(st.list.commands.get_ptr(i as size_t)[0].baseline)
}
public func display_path_count(st : &DisplayState) : u32 { return st.list.paths.size() as u32 }
public func display_gradient_count(st : &DisplayState) : u32 { return st.list.gradients.size() as u32 }
} // namespace ui_internal
