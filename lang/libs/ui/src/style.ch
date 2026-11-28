// P1.3c compact Style id backed by a bounded thread-local StyleData store.
using std::string_view;
using std::vector;

const STYLE_CAP : u32 = 512u32
func space_px(s : Space) : float { switch(s) { Space.None => return 0.0f; Space.XXS => return 2.0f; Space.XS => return 4.0f; Space.SM => return 8.0f; Space.MD => return 12.0f; Space.LG => return 16.0f; Space.XL => return 24.0f; Space.XXL => return 32.0f; Space.XXXL => return 48.0f } return 0.0f }
func dim_equal(a : Dim, b : Dim) : bool { switch(a) { Auto() => { switch(b) { Auto() => return true; default => return false } } Px(x) => { switch(b) { Px(y) => return x == y; default => return false } } Percent(x) => { switch(b) { Percent(y) => return x == y; default => return false } } Fill() => { switch(b) { Fill() => return true; default => return false } } } return false }
func pos_equal(a : Position, b : Position) : bool { switch(a) { Relative() => { switch(b) { Relative() => return true; default => return false } } Absolute(t,r,bo,l) => { switch(b) { Absolute(t2,r2,b2,l2) => return dim_equal(t,t2) && dim_equal(r,r2) && dim_equal(bo,b2) && dim_equal(l,l2); default => return false } } } return false }
func text_style_equal(a : TextStyle, b : TextStyle) : bool { if(a.family != b.family || a.weight != b.weight || a.italic != b.italic || a.size != b.size || a.line_height != b.line_height || a.letter_spacing != b.letter_spacing || a.tab_size != b.tab_size) { return false } if(!a.family_name.equals(&b.family_name)) { return false } if(a.features.ligatures != b.features.ligatures || a.features.tabular_numbers != b.features.tabular_numbers || a.features.slashed_zero != b.features.slashed_zero || a.features.stylistic_set != b.features.stylistic_set) { return false } return true }
@direct_init
struct StyleData {
    internal var f_padding : Insets
    internal var f_padding_space : Space
    internal var f_padding_from_space : bool
    internal var f_gap : Space
    internal var f_width : Dim
    internal var f_height : Dim
    internal var f_min_width : float
    internal var f_max_width : float
    internal var f_grow : float
    internal var f_shrink : float
    internal var f_basis : Dim
    internal var f_align_items : Align
    internal var f_justify : Justify
    internal var f_align_self : Align
    internal var f_background : ColorToken
    internal var f_foreground : ColorToken
    internal var f_border : Border
    internal var f_radius : Radius
    internal var f_shadow : Elevation
    internal var f_opacity : float
    internal var f_clip : bool
    internal var f_overflow : Overflow
    internal var f_text : TextRole
    internal var f_weight : FontWeight
    internal var f_cursor : Cursor
    internal var f_z : i32
    internal var f_aspect : float
    internal var f_position : Position
    internal var f_inset : Insets
    internal var f_grid_col : u16
    internal var f_grid_col_span : u16
    internal var f_grid_row : u16
    internal var f_grid_row_span : u16
    internal var f_direction : Direction
    internal var f_text_style : TextStyle
    internal var f_text_align : TextAlign
    internal var f_wrap : WrapMode
    internal var f_max_lines : u16
    internal var f_text_overflow : TextOverflow
    internal var f_tab_size : u8
    internal var set_mask : u64
}
func make_default_style_data() : StyleData {
    return StyleData {
        f_padding : Insets.zero(), f_padding_space : Space.None, f_padding_from_space : false, f_gap : Space.None, f_width : Dim.Auto(), f_height : Dim.Auto(), f_min_width : 0.0f, f_max_width : 0.0f, f_grow : 0.0f, f_shrink : 0.0f, f_basis : Dim.Auto(), f_align_items : Align.Start, f_justify : Justify.Start, f_align_self : Align.Start, f_background : ColorToken.Transparent, f_foreground : ColorToken.OnSurface, f_border : Border { width : 0.0f, color : ColorToken.Transparent }, f_radius : Radius.None, f_shadow : Elevation.None, f_opacity : 1.0f, f_clip : false, f_overflow : Overflow.Visible, f_text : TextRole.Body, f_weight : FontWeight.Regular, f_cursor : Cursor.Arrow, f_z : 0i32, f_aspect : 0.0f, f_position : Position.Relative(), f_inset : Insets.zero(), f_grid_col : 0u16, f_grid_col_span : 1u16, f_grid_row : 0u16, f_grid_row_span : 1u16, f_direction : Direction.Auto,
        f_text_style : TextStyle { family : FontFamily.Ui, family_name : "", size : 14.0f, weight : FontWeight.Regular, italic : false, line_height : 20.0f / 14.0f, letter_spacing : 0.0f, features : TextFeatures { ligatures : false, tabular_numbers : false, slashed_zero : false, stylistic_set : 0u8, raw_tags : zeroed<[0]u32>() }, tab_size : 4u8 },
        f_text_align : TextAlign.Start, f_wrap : WrapMode.NoWrap, f_max_lines : 0u16, f_text_overflow : TextOverflow.Clip, f_tab_size : 4u8, set_mask : 0u64
    }
}
@thread_local
var STYLE_STORE : [512]StyleData = zeroed:unsafe<[512]StyleData>()
@thread_local var STYLE_COUNT : u32 = 0u32
@thread_local var STYLE_HITS : u32 = 0u32
@thread_local var STYLE_MISSES : u32 = 0u32
@thread_local var STYLE_OVERFLOW : u32 = 0u32
func style_init() { if(STYLE_COUNT == 0u32) { STYLE_STORE[0] = make_default_style_data(); STYLE_COUNT = 1u32 } }
func style_data(id : u32) : StyleData { style_init(); if(id >= STYLE_COUNT) { return STYLE_STORE[0] } return STYLE_STORE[id as size_t] }
func style_data_equal(a : &StyleData, b : &StyleData) : bool { return (a.set_mask == b.set_mask &&
           (a.f_padding.left == b.f_padding.left && a.f_padding.top == b.f_padding.top && a.f_padding.right == b.f_padding.right && a.f_padding.bottom == b.f_padding.bottom) &&
           a.f_padding_space == b.f_padding_space &&
           a.f_padding_from_space == b.f_padding_from_space &&
           a.f_gap == b.f_gap &&
           dim_equal(a.f_width, b.f_width) &&
           dim_equal(a.f_height, b.f_height) &&
           a.f_min_width == b.f_min_width &&
           a.f_max_width == b.f_max_width &&
           a.f_grow == b.f_grow &&
           a.f_shrink == b.f_shrink &&
           dim_equal(a.f_basis, b.f_basis) &&
           a.f_align_items == b.f_align_items &&
           a.f_justify == b.f_justify &&
           a.f_align_self == b.f_align_self &&
           a.f_background == b.f_background &&
           a.f_foreground == b.f_foreground &&
           (a.f_border.width == b.f_border.width && a.f_border.color == b.f_border.color) &&
           a.f_radius == b.f_radius &&
           a.f_shadow == b.f_shadow &&
           a.f_opacity == b.f_opacity &&
           a.f_clip == b.f_clip &&
           a.f_overflow == b.f_overflow &&
           a.f_text == b.f_text &&
           a.f_weight == b.f_weight &&
           a.f_cursor == b.f_cursor &&
           a.f_z == b.f_z &&
           a.f_aspect == b.f_aspect &&
           pos_equal(a.f_position, b.f_position) &&
           (a.f_inset.left == b.f_inset.left && a.f_inset.top == b.f_inset.top && a.f_inset.right == b.f_inset.right && a.f_inset.bottom == b.f_inset.bottom) &&
           a.f_grid_col == b.f_grid_col &&
           a.f_grid_col_span == b.f_grid_col_span &&
           a.f_grid_row == b.f_grid_row &&
           a.f_grid_row_span == b.f_grid_row_span &&
           a.f_direction == b.f_direction &&
           text_style_equal(a.f_text_style, b.f_text_style) &&
           a.f_text_align == b.f_text_align &&
           a.f_wrap == b.f_wrap &&
           a.f_max_lines == b.f_max_lines &&
           a.f_text_overflow == b.f_text_overflow &&
           a.f_tab_size == b.f_tab_size) }
func style_intern(d : StyleData) : Style {
    style_init()
    var i : size_t = 0
    while(i < STYLE_COUNT as size_t) { if(style_data_equal(&STYLE_STORE[i], &d)) { STYLE_HITS = STYLE_HITS + 1u32; return Style { id : i as u32 } } i = i + 1u }
    if(STYLE_COUNT >= STYLE_CAP) { STYLE_OVERFLOW = STYLE_OVERFLOW + 1u32; return Style { id : 0u32 } }
    var idx = STYLE_COUNT; STYLE_STORE[idx as size_t] = d; STYLE_COUNT = STYLE_COUNT + 1u32; STYLE_MISSES = STYLE_MISSES + 1u32; return Style { id : idx }
}
func style_mask_by_id(id : u32) : u64 { style_init(); if(id >= STYLE_COUNT) { return 0u64 }; return STYLE_STORE[id as size_t].set_mask }
@direct_init
public struct Style {
    internal var id : u32
    public func new() : Style { style_init(); return Style { id : 0u32 } }
    func id_internal(&self) : u32 { return self.id }
    public func padding(&self, p : Insets) : Style {
        var d = style_data(self.id)
        d.f_padding = p
        d.f_padding_from_space = false
        d.set_mask = d.set_mask | (1u64 << 0u64)
        return style_intern(d)
    }

    public func gap(&self, s : Space) : Style {
        var d = style_data(self.id)
        d.f_gap = s
        d.set_mask = d.set_mask | (1u64 << 1u64)
        return style_intern(d)
    }

    public func width(&self, s : Dim) : Style {
        var d = style_data(self.id)
        d.f_width = s
        d.set_mask = d.set_mask | (1u64 << 2u64)
        return style_intern(d)
    }

    public func height(&self, s : Dim) : Style {
        var d = style_data(self.id)
        d.f_height = s
        d.set_mask = d.set_mask | (1u64 << 3u64)
        return style_intern(d)
    }

    public func min_width(&self, px : float) : Style {
        var d = style_data(self.id)
        d.f_min_width = px
        d.set_mask = d.set_mask | (1u64 << 4u64)
        return style_intern(d)
    }

    public func max_width(&self, px : float) : Style {
        var d = style_data(self.id)
        d.f_max_width = px
        d.set_mask = d.set_mask | (1u64 << 5u64)
        return style_intern(d)
    }

    public func grow(&self, f : float) : Style {
        var d = style_data(self.id)
        d.f_grow = f
        d.set_mask = d.set_mask | (1u64 << 6u64)
        return style_intern(d)
    }

    public func shrink(&self, f : float) : Style {
        var d = style_data(self.id)
        d.f_shrink = f
        d.set_mask = d.set_mask | (1u64 << 7u64)
        return style_intern(d)
    }

    public func basis(&self, s : Dim) : Style {
        var d = style_data(self.id)
        d.f_basis = s
        d.set_mask = d.set_mask | (1u64 << 8u64)
        return style_intern(d)
    }

    public func align_items(&self, a : Align) : Style {
        var d = style_data(self.id)
        d.f_align_items = a
        d.set_mask = d.set_mask | (1u64 << 9u64)
        return style_intern(d)
    }

    public func justify(&self, j : Justify) : Style {
        var d = style_data(self.id)
        d.f_justify = j
        d.set_mask = d.set_mask | (1u64 << 10u64)
        return style_intern(d)
    }

    public func align_self(&self, a : Align) : Style {
        var d = style_data(self.id)
        d.f_align_self = a
        d.set_mask = d.set_mask | (1u64 << 11u64)
        return style_intern(d)
    }

    public func background(&self, c : ColorToken) : Style {
        var d = style_data(self.id)
        d.f_background = c
        d.set_mask = d.set_mask | (1u64 << 12u64)
        return style_intern(d)
    }

    public func foreground(&self, c : ColorToken) : Style {
        var d = style_data(self.id)
        d.f_foreground = c
        d.set_mask = d.set_mask | (1u64 << 13u64)
        return style_intern(d)
    }

    public func border(&self, b : Border) : Style {
        var d = style_data(self.id)
        d.f_border = b
        d.set_mask = d.set_mask | (1u64 << 14u64)
        return style_intern(d)
    }

    public func radius(&self, r : Radius) : Style {
        var d = style_data(self.id)
        d.f_radius = r
        d.set_mask = d.set_mask | (1u64 << 15u64)
        return style_intern(d)
    }

    public func shadow(&self, e : Elevation) : Style {
        var d = style_data(self.id)
        d.f_shadow = e
        d.set_mask = d.set_mask | (1u64 << 16u64)
        return style_intern(d)
    }

    public func opacity(&self, f : float) : Style {
        var d = style_data(self.id)
        d.f_opacity = f
        d.set_mask = d.set_mask | (1u64 << 17u64)
        return style_intern(d)
    }

    public func clip(&self, on : bool) : Style {
        var d = style_data(self.id)
        d.f_clip = on
        d.set_mask = d.set_mask | (1u64 << 18u64)
        return style_intern(d)
    }

    public func overflow(&self, o : Overflow) : Style {
        var d = style_data(self.id)
        d.f_overflow = o
        d.set_mask = d.set_mask | (1u64 << 19u64)
        return style_intern(d)
    }

    public func text(&self, role : TextRole) : Style {
        var d = style_data(self.id)
        d.f_text = role
        d.set_mask = d.set_mask | (1u64 << 20u64)
        return style_intern(d)
    }

    public func weight(&self, w : FontWeight) : Style {
        var d = style_data(self.id)
        d.f_weight = w
        d.set_mask = d.set_mask | (1u64 << 21u64)
        return style_intern(d)
    }

    public func cursor(&self, c : Cursor) : Style {
        var d = style_data(self.id)
        d.f_cursor = c
        d.set_mask = d.set_mask | (1u64 << 22u64)
        return style_intern(d)
    }

    public func z(&self, value : i32) : Style {
        var d = style_data(self.id)
        d.f_z = value
        d.set_mask = d.set_mask | (1u64 << 23u64)
        return style_intern(d)
    }

    public func aspect(&self, ratio : float) : Style {
        var d = style_data(self.id)
        d.f_aspect = ratio
        d.set_mask = d.set_mask | (1u64 << 24u64)
        return style_intern(d)
    }

    public func position(&self, p : Position) : Style {
        var d = style_data(self.id)
        d.f_position = p
        d.set_mask = d.set_mask | (1u64 << 25u64)
        return style_intern(d)
    }

    public func inset(&self, i : Insets) : Style {
        var d = style_data(self.id)
        d.f_inset = i
        d.set_mask = d.set_mask | (1u64 << 26u64)
        return style_intern(d)
    }

    public func direction(&self, dir : Direction) : Style {
        var d = style_data(self.id)
        d.f_direction = dir
        d.set_mask = d.set_mask | (1u64 << 31u64)
        return style_intern(d)
    }

    public func text_style(&self, t : TextStyle) : Style {
        var d = style_data(self.id)
        d.f_text_style = t
        d.set_mask = d.set_mask | (1u64 << 32u64)
        return style_intern(d)
    }

    public func text_align(&self, a : TextAlign) : Style {
        var d = style_data(self.id)
        d.f_text_align = a
        d.set_mask = d.set_mask | (1u64 << 33u64)
        return style_intern(d)
    }

    public func wrap(&self, w : WrapMode) : Style {
        var d = style_data(self.id)
        d.f_wrap = w
        d.set_mask = d.set_mask | (1u64 << 34u64)
        return style_intern(d)
    }

    public func max_lines(&self, n : u16) : Style {
        var d = style_data(self.id)
        d.f_max_lines = n
        d.set_mask = d.set_mask | (1u64 << 35u64)
        return style_intern(d)
    }

    public func text_overflow(&self, o : TextOverflow) : Style {
        var d = style_data(self.id)
        d.f_text_overflow = o
        d.set_mask = d.set_mask | (1u64 << 36u64)
        return style_intern(d)
    }

    public func tab_size(&self, spaces : u8) : Style {
        var d = style_data(self.id)
        d.f_tab_size = spaces
        d.set_mask = d.set_mask | (1u64 << 37u64)
        return style_intern(d)
    }

    public func padding_all(&self, s : Space) : Style {
        var d = style_data(self.id)
        d.f_padding = Insets.all(space_px(s))
        d.f_padding_space = s
        d.f_padding_from_space = true
        d.set_mask = d.set_mask | (1u64 << 0u64)
        return style_intern(d)
    }

    public func grid_column(&self, col : u16, span : u16) : Style {
        var d = style_data(self.id)
        d.f_grid_col = col; d.f_grid_col_span = span
        d.set_mask = d.set_mask | (1u64 << 27u64) | (1u64 << 28u64)
        return style_intern(d)
    }

    public func grid_row(&self, row : u16, span : u16) : Style {
        var d = style_data(self.id)
        d.f_grid_row = row; d.f_grid_row_span = span
        d.set_mask = d.set_mask | (1u64 << 29u64) | (1u64 << 30u64)
        return style_intern(d)
    }
    public func merge(&self, overlay : Style) : Style {
        style_init()
        var merged = style_data(self.id)
        var overlay_data = style_data(overlay.id)
        if((overlay_data.set_mask & (1u64 << 0u64)) != 0u64) { merged.f_padding = overlay_data.f_padding; merged.f_padding_space = overlay_data.f_padding_space; merged.f_padding_from_space = overlay_data.f_padding_from_space; merged.set_mask = merged.set_mask | (1u64 << 0u64) }
        if((overlay_data.set_mask & (1u64 << 1u64)) != 0u64) { merged.f_gap = overlay_data.f_gap; merged.set_mask = merged.set_mask | (1u64 << 1u64) }
        if((overlay_data.set_mask & (1u64 << 2u64)) != 0u64) { merged.f_width = overlay_data.f_width; merged.set_mask = merged.set_mask | (1u64 << 2u64) }
        if((overlay_data.set_mask & (1u64 << 3u64)) != 0u64) { merged.f_height = overlay_data.f_height; merged.set_mask = merged.set_mask | (1u64 << 3u64) }
        if((overlay_data.set_mask & (1u64 << 4u64)) != 0u64) { merged.f_min_width = overlay_data.f_min_width; merged.set_mask = merged.set_mask | (1u64 << 4u64) }
        if((overlay_data.set_mask & (1u64 << 5u64)) != 0u64) { merged.f_max_width = overlay_data.f_max_width; merged.set_mask = merged.set_mask | (1u64 << 5u64) }
        if((overlay_data.set_mask & (1u64 << 6u64)) != 0u64) { merged.f_grow = overlay_data.f_grow; merged.set_mask = merged.set_mask | (1u64 << 6u64) }
        if((overlay_data.set_mask & (1u64 << 7u64)) != 0u64) { merged.f_shrink = overlay_data.f_shrink; merged.set_mask = merged.set_mask | (1u64 << 7u64) }
        if((overlay_data.set_mask & (1u64 << 8u64)) != 0u64) { merged.f_basis = overlay_data.f_basis; merged.set_mask = merged.set_mask | (1u64 << 8u64) }
        if((overlay_data.set_mask & (1u64 << 9u64)) != 0u64) { merged.f_align_items = overlay_data.f_align_items; merged.set_mask = merged.set_mask | (1u64 << 9u64) }
        if((overlay_data.set_mask & (1u64 << 10u64)) != 0u64) { merged.f_justify = overlay_data.f_justify; merged.set_mask = merged.set_mask | (1u64 << 10u64) }
        if((overlay_data.set_mask & (1u64 << 11u64)) != 0u64) { merged.f_align_self = overlay_data.f_align_self; merged.set_mask = merged.set_mask | (1u64 << 11u64) }
        if((overlay_data.set_mask & (1u64 << 12u64)) != 0u64) { merged.f_background = overlay_data.f_background; merged.set_mask = merged.set_mask | (1u64 << 12u64) }
        if((overlay_data.set_mask & (1u64 << 13u64)) != 0u64) { merged.f_foreground = overlay_data.f_foreground; merged.set_mask = merged.set_mask | (1u64 << 13u64) }
        if((overlay_data.set_mask & (1u64 << 14u64)) != 0u64) { merged.f_border = overlay_data.f_border; merged.set_mask = merged.set_mask | (1u64 << 14u64) }
        if((overlay_data.set_mask & (1u64 << 15u64)) != 0u64) { merged.f_radius = overlay_data.f_radius; merged.set_mask = merged.set_mask | (1u64 << 15u64) }
        if((overlay_data.set_mask & (1u64 << 16u64)) != 0u64) { merged.f_shadow = overlay_data.f_shadow; merged.set_mask = merged.set_mask | (1u64 << 16u64) }
        if((overlay_data.set_mask & (1u64 << 17u64)) != 0u64) { merged.f_opacity = overlay_data.f_opacity; merged.set_mask = merged.set_mask | (1u64 << 17u64) }
        if((overlay_data.set_mask & (1u64 << 18u64)) != 0u64) { merged.f_clip = overlay_data.f_clip; merged.set_mask = merged.set_mask | (1u64 << 18u64) }
        if((overlay_data.set_mask & (1u64 << 19u64)) != 0u64) { merged.f_overflow = overlay_data.f_overflow; merged.set_mask = merged.set_mask | (1u64 << 19u64) }
        if((overlay_data.set_mask & (1u64 << 20u64)) != 0u64) { merged.f_text = overlay_data.f_text; merged.set_mask = merged.set_mask | (1u64 << 20u64) }
        if((overlay_data.set_mask & (1u64 << 21u64)) != 0u64) { merged.f_weight = overlay_data.f_weight; merged.set_mask = merged.set_mask | (1u64 << 21u64) }
        if((overlay_data.set_mask & (1u64 << 22u64)) != 0u64) { merged.f_cursor = overlay_data.f_cursor; merged.set_mask = merged.set_mask | (1u64 << 22u64) }
        if((overlay_data.set_mask & (1u64 << 23u64)) != 0u64) { merged.f_z = overlay_data.f_z; merged.set_mask = merged.set_mask | (1u64 << 23u64) }
        if((overlay_data.set_mask & (1u64 << 24u64)) != 0u64) { merged.f_aspect = overlay_data.f_aspect; merged.set_mask = merged.set_mask | (1u64 << 24u64) }
        if((overlay_data.set_mask & (1u64 << 25u64)) != 0u64) { merged.f_position = overlay_data.f_position; merged.set_mask = merged.set_mask | (1u64 << 25u64) }
        if((overlay_data.set_mask & (1u64 << 26u64)) != 0u64) { merged.f_inset = overlay_data.f_inset; merged.set_mask = merged.set_mask | (1u64 << 26u64) }
        if((overlay_data.set_mask & (1u64 << 27u64)) != 0u64) { merged.f_grid_col = overlay_data.f_grid_col; merged.set_mask = merged.set_mask | (1u64 << 27u64) }
        if((overlay_data.set_mask & (1u64 << 28u64)) != 0u64) { merged.f_grid_col_span = overlay_data.f_grid_col_span; merged.set_mask = merged.set_mask | (1u64 << 28u64) }
        if((overlay_data.set_mask & (1u64 << 29u64)) != 0u64) { merged.f_grid_row = overlay_data.f_grid_row; merged.set_mask = merged.set_mask | (1u64 << 29u64) }
        if((overlay_data.set_mask & (1u64 << 30u64)) != 0u64) { merged.f_grid_row_span = overlay_data.f_grid_row_span; merged.set_mask = merged.set_mask | (1u64 << 30u64) }
        if((overlay_data.set_mask & (1u64 << 31u64)) != 0u64) { merged.f_direction = overlay_data.f_direction; merged.set_mask = merged.set_mask | (1u64 << 31u64) }
        if((overlay_data.set_mask & (1u64 << 32u64)) != 0u64) { merged.f_text_style = overlay_data.f_text_style; merged.set_mask = merged.set_mask | (1u64 << 32u64) }
        if((overlay_data.set_mask & (1u64 << 33u64)) != 0u64) { merged.f_text_align = overlay_data.f_text_align; merged.set_mask = merged.set_mask | (1u64 << 33u64) }
        if((overlay_data.set_mask & (1u64 << 34u64)) != 0u64) { merged.f_wrap = overlay_data.f_wrap; merged.set_mask = merged.set_mask | (1u64 << 34u64) }
        if((overlay_data.set_mask & (1u64 << 35u64)) != 0u64) { merged.f_max_lines = overlay_data.f_max_lines; merged.set_mask = merged.set_mask | (1u64 << 35u64) }
        if((overlay_data.set_mask & (1u64 << 36u64)) != 0u64) { merged.f_text_overflow = overlay_data.f_text_overflow; merged.set_mask = merged.set_mask | (1u64 << 36u64) }
        if((overlay_data.set_mask & (1u64 << 37u64)) != 0u64) { merged.f_tab_size = overlay_data.f_tab_size; merged.set_mask = merged.set_mask | (1u64 << 37u64) }
        return style_intern(merged)
    }
}
public namespace ui_internal {
public func style_store_reset() { STYLE_COUNT = 0u32; STYLE_HITS = 0u32; STYLE_MISSES = 0u32; STYLE_OVERFLOW = 0u32; style_init() }
public func style_store_size() : u32 { style_init(); return STYLE_COUNT }
public func style_store_hits() : u32 { return STYLE_HITS }
public func style_store_misses() : u32 { return STYLE_MISSES }
public func style_store_overflow() : u32 { return STYLE_OVERFLOW }
public func style_id(s : Style) : u32 { return s.id_internal() }

public func resolve_color(t : &Theme, token : ColorToken) : Color { return t.color(token) }
public func resolve_space(t : &Theme, s : Space) : float { return t.space(s) }
public func resolve_radius(t : &Theme, r : Radius) : float { return t.radius(r) }
public func resolve_elevation(t : &Theme, e : Elevation) : Shadow { return t.elevation(e) }
public func resolve_text(t : &Theme, r : TextRole) : TextStyle { return t.text(r) }
public func resolve_weight(w : FontWeight) : FontWeight { return w }
public func resolve_density(t : &Theme) : Density { return t.density() }

@direct_init
public struct ResolvedStyle {
    var background : Color
    var foreground : Color
    var padding : Insets
    var gap : float
    var radius : float
    var border_width : float
    var border_color : Color
    var shadow : Shadow
    var opacity : float
    var text : TextStyle
    var text_color : Color
    var text_role : TextRole
    var text_weight : FontWeight
    var width : Dim
    var height : Dim
    var min_width : float
    var max_width : float
    var grow : float
    var shrink : float
    var basis : Dim
    var align_items : Align
    var justify : Justify
    var align_self : Align
    var overflow : Overflow
    var clip : bool
    var cursor : Cursor
    var z : i32
    var aspect : float
    var position : Position
    var inset : Insets
    var grid_col : u16
    var grid_col_span : u16
    var grid_row : u16
    var grid_row_span : u16
    var direction : Direction
    var text_align : TextAlign
    var wrap : WrapMode
    var max_lines : u16
    var text_overflow : TextOverflow
    var tab_size : u8
}

@direct_init
public struct StyleStateFlags {
    var hover : bool
    var focus : bool
    var pressed : bool
    var disabled : bool
    var selected : bool
    var checked : bool
    var invalid : bool
}

@direct_init
public struct StyleStateSet {
    var hover : Style
    var focus : Style
    var pressed : Style
    var disabled : Style
    var selected : Style
    var checked : Style
    var invalid : Style
}

public func state_flags_none() : StyleStateFlags {
    return StyleStateFlags { hover : false, focus : false, pressed : false, disabled : false,
        selected : false, checked : false, invalid : false }
}

public func state_styles_empty() : StyleStateSet {
    return StyleStateSet { hover : Style.new(), focus : Style.new(), pressed : Style.new(),
        disabled : Style.new(), selected : Style.new(), checked : Style.new(), invalid : Style.new() }
}

public func style_resolve(s : Style, t : &Theme) : ResolvedStyle {
    var d = style_data(s.id_internal())
    var padding = d.f_padding
    if(d.f_padding_from_space) { padding = Insets.all(t.space(d.f_padding_space)) }
    var text = t.text(d.f_text)
    if((d.set_mask & (1u64 << 32u64)) != 0u64) { text = d.f_text_style }
    if((d.set_mask & (1u64 << 21u64)) != 0u64) { text.weight = d.f_weight }
    if((d.set_mask & (1u64 << 37u64)) != 0u64) { text.tab_size = d.f_tab_size }
    var fg = t.color(d.f_foreground)
    return ResolvedStyle {
        background : t.color(d.f_background), foreground : fg,
        padding : padding, gap : t.space(d.f_gap), radius : t.radius(d.f_radius),
        border_width : d.f_border.width, border_color : t.color(d.f_border.color),
        shadow : t.elevation(d.f_shadow), opacity : d.f_opacity,
        text : text, text_color : fg, text_role : d.f_text, text_weight : text.weight,
        width : d.f_width, height : d.f_height, min_width : d.f_min_width,
        max_width : d.f_max_width, grow : d.f_grow, shrink : d.f_shrink,
        basis : d.f_basis, align_items : d.f_align_items, justify : d.f_justify,
        align_self : d.f_align_self, overflow : d.f_overflow, clip : d.f_clip,
        cursor : d.f_cursor, z : d.f_z, aspect : d.f_aspect, position : d.f_position,
        inset : d.f_inset, grid_col : d.f_grid_col, grid_col_span : d.f_grid_col_span,
        grid_row : d.f_grid_row, grid_row_span : d.f_grid_row_span,
        direction : d.f_direction, text_align : d.f_text_align, wrap : d.f_wrap,
        max_lines : d.f_max_lines, text_overflow : d.f_text_overflow, tab_size : d.f_tab_size
    }
}

// Later states win; disabled is the final override. Invalid is above selected/checked.
public func style_resolve_state(base : Style, states : StyleStateSet, flags : StyleStateFlags, t : &Theme) : ResolvedStyle {
    var merged = base
    if(flags.hover) { merged = merged.merge(states.hover) }
    if(flags.focus) { merged = merged.merge(states.focus) }
    if(flags.pressed) { merged = merged.merge(states.pressed) }
    if(flags.selected) { merged = merged.merge(states.selected) }
    if(flags.checked) { merged = merged.merge(states.checked) }
    if(flags.invalid) { merged = merged.merge(states.invalid) }
    if(flags.disabled) { merged = merged.merge(states.disabled) }
    var r = style_resolve(merged, t)
    if(flags.disabled) { r.opacity = r.opacity * t.disabled_opacity() }
    return r
}
}
