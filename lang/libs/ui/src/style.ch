// P1.3 Style value type. Token resolution/layout/paint semantics stay P1.5/P1.6.
using std::string_view;

func space_px(s : Space) : float {
    switch(s) {
        Space.None => return 0.0f
        Space.XXS => return 2.0f
        Space.XS => return 4.0f
        Space.SM => return 8.0f
        Space.MD => return 12.0f
        Space.LG => return 16.0f
        Space.XL => return 24.0f
        Space.XXL => return 32.0f
        Space.XXXL => return 48.0f
    }
    return 0.0f
}

@direct_init
public struct Style {
    internal var f_padding : Insets
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

    public func new() : Style {
        return Style {
            f_padding : Insets.zero(), f_gap : Space.None, f_width : Dim.Auto(), f_height : Dim.Auto(),
            f_min_width : 0.0f, f_max_width : 0.0f, f_grow : 0.0f, f_shrink : 0.0f, f_basis : Dim.Auto(),
            f_align_items : Align.Start, f_justify : Justify.Start, f_align_self : Align.Start,
            f_background : ColorToken.Transparent, f_foreground : ColorToken.OnSurface,
            f_border : Border { width : 0.0f, color : ColorToken.Transparent },
            f_radius : Radius.None, f_shadow : Elevation.None, f_opacity : 1.0f, f_clip : false,
            f_overflow : Overflow.Visible, f_text : TextRole.Body, f_weight : FontWeight.Regular,
            f_cursor : Cursor.Arrow, f_z : 0i32, f_aspect : 0.0f, f_position : Position.Relative(),
            f_inset : Insets.zero(), f_grid_col : 0u16, f_grid_col_span : 1u16,
            f_grid_row : 0u16, f_grid_row_span : 1u16, f_direction : Direction.Auto,
            f_text_style : TextStyle { family : FontFamily.Ui, family_name : "", size : 14.0f,
                weight : FontWeight.Regular, italic : false, line_height : 20.0f / 14.0f,
                letter_spacing : 0.0f, features : TextFeatures { ligatures : false, tabular_numbers : false, slashed_zero : false, stylistic_set : 0u8, raw_tags : zeroed<[0]u32>() }, tab_size : 4u8 },
            f_text_align : TextAlign.Start, f_wrap : WrapMode.NoWrap, f_max_lines : 0u16,
            f_text_overflow : TextOverflow.Clip, f_tab_size : 4u8, set_mask : 0u64
        }
    }

    public func padding(&self, p : Insets) : Style {
        var s = *self
        s.f_padding = p
        s.set_mask = s.set_mask | (1u64 << 0u64)
        return s
    }

    public func gap(&self, s : Space) : Style {
        var n = *self
        n.f_gap = s
        n.set_mask = n.set_mask | (1u64 << 1u64)
        return n
    }

    public func width(&self, s : Dim) : Style {
        var n = *self
        n.f_width = s
        n.set_mask = n.set_mask | (1u64 << 2u64)
        return n
    }

    public func height(&self, s : Dim) : Style {
        var n = *self
        n.f_height = s
        n.set_mask = n.set_mask | (1u64 << 3u64)
        return n
    }

    public func min_width(&self, px : float) : Style {
        var s = *self
        s.f_min_width = px
        s.set_mask = s.set_mask | (1u64 << 4u64)
        return s
    }

    public func max_width(&self, px : float) : Style {
        var s = *self
        s.f_max_width = px
        s.set_mask = s.set_mask | (1u64 << 5u64)
        return s
    }

    public func grow(&self, f : float) : Style {
        var s = *self
        s.f_grow = f
        s.set_mask = s.set_mask | (1u64 << 6u64)
        return s
    }

    public func shrink(&self, f : float) : Style {
        var s = *self
        s.f_shrink = f
        s.set_mask = s.set_mask | (1u64 << 7u64)
        return s
    }

    public func basis(&self, s : Dim) : Style {
        var n = *self
        n.f_basis = s
        n.set_mask = n.set_mask | (1u64 << 8u64)
        return n
    }

    public func align_items(&self, a : Align) : Style {
        var s = *self
        s.f_align_items = a
        s.set_mask = s.set_mask | (1u64 << 9u64)
        return s
    }

    public func justify(&self, j : Justify) : Style {
        var s = *self
        s.f_justify = j
        s.set_mask = s.set_mask | (1u64 << 10u64)
        return s
    }

    public func align_self(&self, a : Align) : Style {
        var s = *self
        s.f_align_self = a
        s.set_mask = s.set_mask | (1u64 << 11u64)
        return s
    }

    public func background(&self, c : ColorToken) : Style {
        var s = *self
        s.f_background = c
        s.set_mask = s.set_mask | (1u64 << 12u64)
        return s
    }

    public func foreground(&self, c : ColorToken) : Style {
        var s = *self
        s.f_foreground = c
        s.set_mask = s.set_mask | (1u64 << 13u64)
        return s
    }

    public func border(&self, b : Border) : Style {
        var s = *self
        s.f_border = b
        s.set_mask = s.set_mask | (1u64 << 14u64)
        return s
    }

    public func radius(&self, r : Radius) : Style {
        var s = *self
        s.f_radius = r
        s.set_mask = s.set_mask | (1u64 << 15u64)
        return s
    }

    public func shadow(&self, e : Elevation) : Style {
        var s = *self
        s.f_shadow = e
        s.set_mask = s.set_mask | (1u64 << 16u64)
        return s
    }

    public func opacity(&self, f : float) : Style {
        var s = *self
        s.f_opacity = f
        s.set_mask = s.set_mask | (1u64 << 17u64)
        return s
    }

    public func clip(&self, on : bool) : Style {
        var s = *self
        s.f_clip = on
        s.set_mask = s.set_mask | (1u64 << 18u64)
        return s
    }

    public func overflow(&self, o : Overflow) : Style {
        var s = *self
        s.f_overflow = o
        s.set_mask = s.set_mask | (1u64 << 19u64)
        return s
    }

    public func text(&self, role : TextRole) : Style {
        var s = *self
        s.f_text = role
        s.set_mask = s.set_mask | (1u64 << 20u64)
        return s
    }

    public func weight(&self, w : FontWeight) : Style {
        var s = *self
        s.f_weight = w
        s.set_mask = s.set_mask | (1u64 << 21u64)
        return s
    }

    public func cursor(&self, c : Cursor) : Style {
        var s = *self
        s.f_cursor = c
        s.set_mask = s.set_mask | (1u64 << 22u64)
        return s
    }

    public func z(&self, value : i32) : Style {
        var s = *self
        s.f_z = value
        s.set_mask = s.set_mask | (1u64 << 23u64)
        return s
    }

    public func aspect(&self, ratio : float) : Style {
        var s = *self
        s.f_aspect = ratio
        s.set_mask = s.set_mask | (1u64 << 24u64)
        return s
    }

    public func position(&self, p : Position) : Style {
        var s = *self
        s.f_position = p
        s.set_mask = s.set_mask | (1u64 << 25u64)
        return s
    }

    public func inset(&self, i : Insets) : Style {
        var s = *self
        s.f_inset = i
        s.set_mask = s.set_mask | (1u64 << 26u64)
        return s
    }

    public func grid_col(&self, col : u16) : Style {
        var s = *self
        s.f_grid_col = col
        s.set_mask = s.set_mask | (1u64 << 27u64)
        return s
    }

    public func grid_col_span(&self, span : u16) : Style {
        var s = *self
        s.f_grid_col_span = span
        s.set_mask = s.set_mask | (1u64 << 28u64)
        return s
    }





    public func direction(&self, d : Direction) : Style {
        var s = *self
        s.f_direction = d
        s.set_mask = s.set_mask | (1u64 << 31u64)
        return s
    }

    public func text_style(&self, t : TextStyle) : Style {
        var s = *self
        s.f_text_style = t
        s.set_mask = s.set_mask | (1u64 << 32u64)
        return s
    }

    public func text_align(&self, a : TextAlign) : Style {
        var s = *self
        s.f_text_align = a
        s.set_mask = s.set_mask | (1u64 << 33u64)
        return s
    }

    public func wrap(&self, w : WrapMode) : Style {
        var s = *self
        s.f_wrap = w
        s.set_mask = s.set_mask | (1u64 << 34u64)
        return s
    }

    public func max_lines(&self, n : u16) : Style {
        var s = *self
        s.f_max_lines = n
        s.set_mask = s.set_mask | (1u64 << 35u64)
        return s
    }

    public func text_overflow(&self, o : TextOverflow) : Style {
        var s = *self
        s.f_text_overflow = o
        s.set_mask = s.set_mask | (1u64 << 36u64)
        return s
    }

    public func tab_size(&self, spaces : u8) : Style {
        var s = *self
        s.f_tab_size = spaces
        s.set_mask = s.set_mask | (1u64 << 37u64)
        return s
    }

    public func padding_all(&self, s : Space) : Style {
        var n = *self
        n.f_padding = Insets.all(space_px(s))
        n.set_mask = n.set_mask | (1u64 << 0u64)
        return n
    }

    public func grid_column(&self, col : u16, span : u16) : Style {
        var s = *self
        s.f_grid_col = col
        s.f_grid_col_span = span
        s.set_mask = s.set_mask | (1u64 << 27u64) | (1u64 << 28u64)
        return s
    }

    public func grid_row(&self, row : u16, span : u16) : Style {
        var s = *self
        s.f_grid_row = row
        s.f_grid_row_span = span
        s.set_mask = s.set_mask | (1u64 << 29u64) | (1u64 << 30u64)
        return s
    }

    public func merge(&self, overlay : Style) : Style {
        var merged = *self
        if((overlay.set_mask & (1u64 << 0u64)) != 0u64) { merged.f_padding = overlay.f_padding; merged.set_mask = merged.set_mask | (1u64 << 0u64) }
        if((overlay.set_mask & (1u64 << 1u64)) != 0u64) { merged.f_gap = overlay.f_gap; merged.set_mask = merged.set_mask | (1u64 << 1u64) }
        if((overlay.set_mask & (1u64 << 2u64)) != 0u64) { merged.f_width = overlay.f_width; merged.set_mask = merged.set_mask | (1u64 << 2u64) }
        if((overlay.set_mask & (1u64 << 3u64)) != 0u64) { merged.f_height = overlay.f_height; merged.set_mask = merged.set_mask | (1u64 << 3u64) }
        if((overlay.set_mask & (1u64 << 4u64)) != 0u64) { merged.f_min_width = overlay.f_min_width; merged.set_mask = merged.set_mask | (1u64 << 4u64) }
        if((overlay.set_mask & (1u64 << 5u64)) != 0u64) { merged.f_max_width = overlay.f_max_width; merged.set_mask = merged.set_mask | (1u64 << 5u64) }
        if((overlay.set_mask & (1u64 << 6u64)) != 0u64) { merged.f_grow = overlay.f_grow; merged.set_mask = merged.set_mask | (1u64 << 6u64) }
        if((overlay.set_mask & (1u64 << 7u64)) != 0u64) { merged.f_shrink = overlay.f_shrink; merged.set_mask = merged.set_mask | (1u64 << 7u64) }
        if((overlay.set_mask & (1u64 << 8u64)) != 0u64) { merged.f_basis = overlay.f_basis; merged.set_mask = merged.set_mask | (1u64 << 8u64) }
        if((overlay.set_mask & (1u64 << 9u64)) != 0u64) { merged.f_align_items = overlay.f_align_items; merged.set_mask = merged.set_mask | (1u64 << 9u64) }
        if((overlay.set_mask & (1u64 << 10u64)) != 0u64) { merged.f_justify = overlay.f_justify; merged.set_mask = merged.set_mask | (1u64 << 10u64) }
        if((overlay.set_mask & (1u64 << 11u64)) != 0u64) { merged.f_align_self = overlay.f_align_self; merged.set_mask = merged.set_mask | (1u64 << 11u64) }
        if((overlay.set_mask & (1u64 << 12u64)) != 0u64) { merged.f_background = overlay.f_background; merged.set_mask = merged.set_mask | (1u64 << 12u64) }
        if((overlay.set_mask & (1u64 << 13u64)) != 0u64) { merged.f_foreground = overlay.f_foreground; merged.set_mask = merged.set_mask | (1u64 << 13u64) }
        if((overlay.set_mask & (1u64 << 14u64)) != 0u64) { merged.f_border = overlay.f_border; merged.set_mask = merged.set_mask | (1u64 << 14u64) }
        if((overlay.set_mask & (1u64 << 15u64)) != 0u64) { merged.f_radius = overlay.f_radius; merged.set_mask = merged.set_mask | (1u64 << 15u64) }
        if((overlay.set_mask & (1u64 << 16u64)) != 0u64) { merged.f_shadow = overlay.f_shadow; merged.set_mask = merged.set_mask | (1u64 << 16u64) }
        if((overlay.set_mask & (1u64 << 17u64)) != 0u64) { merged.f_opacity = overlay.f_opacity; merged.set_mask = merged.set_mask | (1u64 << 17u64) }
        if((overlay.set_mask & (1u64 << 18u64)) != 0u64) { merged.f_clip = overlay.f_clip; merged.set_mask = merged.set_mask | (1u64 << 18u64) }
        if((overlay.set_mask & (1u64 << 19u64)) != 0u64) { merged.f_overflow = overlay.f_overflow; merged.set_mask = merged.set_mask | (1u64 << 19u64) }
        if((overlay.set_mask & (1u64 << 20u64)) != 0u64) { merged.f_text = overlay.f_text; merged.set_mask = merged.set_mask | (1u64 << 20u64) }
        if((overlay.set_mask & (1u64 << 21u64)) != 0u64) { merged.f_weight = overlay.f_weight; merged.set_mask = merged.set_mask | (1u64 << 21u64) }
        if((overlay.set_mask & (1u64 << 22u64)) != 0u64) { merged.f_cursor = overlay.f_cursor; merged.set_mask = merged.set_mask | (1u64 << 22u64) }
        if((overlay.set_mask & (1u64 << 23u64)) != 0u64) { merged.f_z = overlay.f_z; merged.set_mask = merged.set_mask | (1u64 << 23u64) }
        if((overlay.set_mask & (1u64 << 24u64)) != 0u64) { merged.f_aspect = overlay.f_aspect; merged.set_mask = merged.set_mask | (1u64 << 24u64) }
        if((overlay.set_mask & (1u64 << 25u64)) != 0u64) { merged.f_position = overlay.f_position; merged.set_mask = merged.set_mask | (1u64 << 25u64) }
        if((overlay.set_mask & (1u64 << 26u64)) != 0u64) { merged.f_inset = overlay.f_inset; merged.set_mask = merged.set_mask | (1u64 << 26u64) }
        if((overlay.set_mask & (1u64 << 27u64)) != 0u64) { merged.f_grid_col = overlay.f_grid_col; merged.set_mask = merged.set_mask | (1u64 << 27u64) }
        if((overlay.set_mask & (1u64 << 28u64)) != 0u64) { merged.f_grid_col_span = overlay.f_grid_col_span; merged.set_mask = merged.set_mask | (1u64 << 28u64) }
        if((overlay.set_mask & (1u64 << 29u64)) != 0u64) { merged.f_grid_row = overlay.f_grid_row; merged.set_mask = merged.set_mask | (1u64 << 29u64) }
        if((overlay.set_mask & (1u64 << 30u64)) != 0u64) { merged.f_grid_row_span = overlay.f_grid_row_span; merged.set_mask = merged.set_mask | (1u64 << 30u64) }
        if((overlay.set_mask & (1u64 << 31u64)) != 0u64) { merged.f_direction = overlay.f_direction; merged.set_mask = merged.set_mask | (1u64 << 31u64) }
        if((overlay.set_mask & (1u64 << 32u64)) != 0u64) { merged.f_text_style = overlay.f_text_style; merged.set_mask = merged.set_mask | (1u64 << 32u64) }
        if((overlay.set_mask & (1u64 << 33u64)) != 0u64) { merged.f_text_align = overlay.f_text_align; merged.set_mask = merged.set_mask | (1u64 << 33u64) }
        if((overlay.set_mask & (1u64 << 34u64)) != 0u64) { merged.f_wrap = overlay.f_wrap; merged.set_mask = merged.set_mask | (1u64 << 34u64) }
        if((overlay.set_mask & (1u64 << 35u64)) != 0u64) { merged.f_max_lines = overlay.f_max_lines; merged.set_mask = merged.set_mask | (1u64 << 35u64) }
        if((overlay.set_mask & (1u64 << 36u64)) != 0u64) { merged.f_text_overflow = overlay.f_text_overflow; merged.set_mask = merged.set_mask | (1u64 << 36u64) }
        if((overlay.set_mask & (1u64 << 37u64)) != 0u64) { merged.f_tab_size = overlay.f_tab_size; merged.set_mask = merged.set_mask | (1u64 << 37u64) }
        return merged
    }
}
