import "@std/hashing/fnv1.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"

func getFontWeightKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("normal") => {
            return CSSKeywordKind.Normal;
        }
        comptime_fnv1_hash("bold") => {
            return CSSKeywordKind.Bold
        }
        comptime_fnv1_hash("bolder") => {
            return CSSKeywordKind.Bolder
        }
        comptime_fnv1_hash("lighter") => {
            return CSSKeywordKind.Lighter
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getFontStyleKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("normal") => {
            return CSSKeywordKind.Normal;
        }
        comptime_fnv1_hash("italic") => {
            return CSSKeywordKind.Italic
        }
        comptime_fnv1_hash("oblique") => {
            return CSSKeywordKind.Oblique
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getFontVariantKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("normal") => {
            return CSSKeywordKind.Normal;
        }
        comptime_fnv1_hash("small-caps") => {
            return CSSKeywordKind.SmallCaps
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getListStyleTypeKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("disc") => {
            return CSSKeywordKind.Disc;
        }
        comptime_fnv1_hash("circle") => {
            return CSSKeywordKind.Circle
        }
        comptime_fnv1_hash("square") => {
            return CSSKeywordKind.Square
        }
        comptime_fnv1_hash("decimal") => {
            return CSSKeywordKind.Decimal
        }
        comptime_fnv1_hash("decimal-leading-zero") => {
            return CSSKeywordKind.DecimalLeadingZero
        }
        comptime_fnv1_hash("lower-roman") => {
            return CSSKeywordKind.LowerRoman
        }
        comptime_fnv1_hash("upper-roman") => {
            return CSSKeywordKind.UpperRoman
        }
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getListStylePositionKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("inside") => {
            return CSSKeywordKind.Inside;
        }
        comptime_fnv1_hash("outside") => {
            return CSSKeywordKind.Outside
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getAlignItemsKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("stretch") => {
            return CSSKeywordKind.Stretch;
        }
        comptime_fnv1_hash("flex-start") => {
            return CSSKeywordKind.FlexStart
        }
        comptime_fnv1_hash("flex-end") => {
            return CSSKeywordKind.FlexEnd
        }
        comptime_fnv1_hash("center") => {
            return CSSKeywordKind.Center
        }
        comptime_fnv1_hash("baseline") => {
            return CSSKeywordKind.Baseline
        }
        comptime_fnv1_hash("start") => {
            return CSSKeywordKind.Start
        }
        comptime_fnv1_hash("end") => {
            return CSSKeywordKind.End
        }
        comptime_fnv1_hash("self-start") => {
            return CSSKeywordKind.SelfStart
        }
        comptime_fnv1_hash("self-end") => {
            return CSSKeywordKind.SelfEnd
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getFontSizeKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("xx-small") => {
            return CSSKeywordKind.XXSmall;
        }
        comptime_fnv1_hash("x-small") => {
            return CSSKeywordKind.XSmall
        }
        comptime_fnv1_hash("small") => {
            return CSSKeywordKind.Small
        }
        comptime_fnv1_hash("medium") => {
            return CSSKeywordKind.Medium
        }
        comptime_fnv1_hash("large") => {
            return CSSKeywordKind.Large
        }
        comptime_fnv1_hash("x-large") => {
            return CSSKeywordKind.XLarge
        }
        comptime_fnv1_hash("xx-large") => {
            return CSSKeywordKind.XXLarge
        }
        comptime_fnv1_hash("smaller") => {
            return CSSKeywordKind.Smaller
        }
        comptime_fnv1_hash("larger") => {
            return CSSKeywordKind.Larger
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}


func getTextAlignKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("left") => {
            return CSSKeywordKind.Left;
        }
        comptime_fnv1_hash("right") => {
            return CSSKeywordKind.Right
        }
        comptime_fnv1_hash("center") => {
            return CSSKeywordKind.Center
        }
        comptime_fnv1_hash("justify") => {
            return CSSKeywordKind.Justify
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getDisplayKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None;
        }
        comptime_fnv1_hash("inline") => {
            return CSSKeywordKind.Inline
        }
        comptime_fnv1_hash("block") => {
            return CSSKeywordKind.Block
        }
        comptime_fnv1_hash("inline-block") => {
            return CSSKeywordKind.InlineBlock
        }
        comptime_fnv1_hash("flex") => {
            return CSSKeywordKind.Flex
        }
        comptime_fnv1_hash("grid") => {
            return CSSKeywordKind.Grid
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getPositionKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("static") => {
            return CSSKeywordKind.Static;
        }
        comptime_fnv1_hash("relative") => {
            return CSSKeywordKind.Relative
        }
        comptime_fnv1_hash("absolute") => {
            return CSSKeywordKind.Absolute
        }
        comptime_fnv1_hash("fixed") => {
            return CSSKeywordKind.Fixed
        }
        comptime_fnv1_hash("sticky") => {
            return CSSKeywordKind.Sticky
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getOverflowKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("visible") => {
            return CSSKeywordKind.Visible;
        }
        comptime_fnv1_hash("hidden") => {
            return CSSKeywordKind.Hidden
        }
        comptime_fnv1_hash("scroll") => {
            return CSSKeywordKind.Scroll
        }
        comptime_fnv1_hash("auto") => {
            return CSSKeywordKind.Auto
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getFloatKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("left") => {
            return CSSKeywordKind.Left;
        }
        comptime_fnv1_hash("right") => {
            return CSSKeywordKind.Right
        }
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getClearKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None;
        }
        comptime_fnv1_hash("left") => {
            return CSSKeywordKind.Left
        }
        comptime_fnv1_hash("right") => {
            return CSSKeywordKind.Right
        }
        comptime_fnv1_hash("both") => {
            return CSSKeywordKind.Both
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getVerticalAlignKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("baseline") => {
            return CSSKeywordKind.Baseline;
        }
        comptime_fnv1_hash("sub") => {
            return CSSKeywordKind.Sub
        }
        comptime_fnv1_hash("super") => {
            return CSSKeywordKind.Super
        }
        comptime_fnv1_hash("text-top") => {
            return CSSKeywordKind.TextTop
        }
        comptime_fnv1_hash("text-bottom") => {
            return CSSKeywordKind.TextBottom
        }
        comptime_fnv1_hash("middle") => {
            return CSSKeywordKind.Middle
        }
        comptime_fnv1_hash("top") => {
            return CSSKeywordKind.Top
        }
        comptime_fnv1_hash("bottom") => {
            return CSSKeywordKind.Bottom
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getWhitespaceKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("normal") => {
            return CSSKeywordKind.Normal;
        }
        comptime_fnv1_hash("nowrap") => {
            return CSSKeywordKind.Nowrap
        }
        comptime_fnv1_hash("pre") => {
            return CSSKeywordKind.Pre
        }
        comptime_fnv1_hash("pre-wrap") => {
            return CSSKeywordKind.PreWrap
        }
        comptime_fnv1_hash("pre-line") => {
            return CSSKeywordKind.PreLine
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getTextTransformKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None;
        }
        comptime_fnv1_hash("capitalize") => {
            return CSSKeywordKind.Capitalize
        }
        comptime_fnv1_hash("uppercase") => {
            return CSSKeywordKind.Uppercase
        }
        comptime_fnv1_hash("lowercase") => {
            return CSSKeywordKind.Lowercase
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getVisibilityKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("visible") => {
            return CSSKeywordKind.Visible;
        }
        comptime_fnv1_hash("hidden") => {
            return CSSKeywordKind.Hidden
        }
        comptime_fnv1_hash("collapse") => {
            return CSSKeywordKind.Collapse
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getCursorKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("auto") => {
            return CSSKeywordKind.Auto;
        }
        comptime_fnv1_hash("default") => {
            return CSSKeywordKind.Default
        }
        comptime_fnv1_hash("pointer") => {
            return CSSKeywordKind.Pointer
        }
        comptime_fnv1_hash("move") => {
            return CSSKeywordKind.Move
        }
        comptime_fnv1_hash("text") => {
            return CSSKeywordKind.Text
        }
        comptime_fnv1_hash("wait") => {
            return CSSKeywordKind.Wait
        }
        comptime_fnv1_hash("help") => {
            return CSSKeywordKind.Help
        }
        comptime_fnv1_hash("not-allowed") => {
            return CSSKeywordKind.NotAllowed
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getDirectionKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("ltr") => {
            return CSSKeywordKind.Ltr;
        }
        comptime_fnv1_hash("rtl") => {
            return CSSKeywordKind.Rtl
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getResizeKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None;
        }
        comptime_fnv1_hash("both") => {
            return CSSKeywordKind.Both
        }
        comptime_fnv1_hash("horizontal") => {
            return CSSKeywordKind.Horizontal
        }
        comptime_fnv1_hash("vertical") => {
            return CSSKeywordKind.Vertical
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getTableLayoutKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("auto") => {
            return CSSKeywordKind.Auto;
        }
        comptime_fnv1_hash("fixed") => {
            return CSSKeywordKind.Fixed
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getBorderCollapseKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("collapse") => {
            return CSSKeywordKind.Collapse;
        }
        comptime_fnv1_hash("separate") => {
            return CSSKeywordKind.Separate
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getTextOverflowKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("clip") => {
            return CSSKeywordKind.Clip;
        }
        comptime_fnv1_hash("ellipsis") => {
            return CSSKeywordKind.Ellipsis
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getOverflowWrapKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("normal") => {
            return CSSKeywordKind.Normal;
        }
        comptime_fnv1_hash("break-word") => {
            return CSSKeywordKind.BreakWord
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getWordBreakKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("normal") => {
            return CSSKeywordKind.Normal;
        }
        comptime_fnv1_hash("break-all") => {
            return CSSKeywordKind.BreakAll;
        }
        comptime_fnv1_hash("keep-all") => {
            return CSSKeywordKind.KeepAll;
        }
        comptime_fnv1_hash("break-word") => {
            return CSSKeywordKind.BreakWord
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getObjectFitKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("fill") => {
            return CSSKeywordKind.Fill;
        }
        comptime_fnv1_hash("contain") => {
            return CSSKeywordKind.Contain;
        }
        comptime_fnv1_hash("cover") => {
            return CSSKeywordKind.Cover;
        }
        comptime_fnv1_hash("none") => {
            return CSSKeywordKind.None
        }
        comptime_fnv1_hash("scale-down") => {
            return CSSKeywordKind.ScaleDown
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getImageRenderingKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("auto") => {
            return CSSKeywordKind.Auto;
        }
        comptime_fnv1_hash("crisp-edges") => {
            return CSSKeywordKind.CrispEdges;
        }
        comptime_fnv1_hash("pixelated") => {
            return CSSKeywordKind.Pixelated;
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func getBackFaceVisibilityKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("visible") => {
            return CSSKeywordKind.Visible;
        }
        comptime_fnv1_hash("hidden") => {
            return CSSKeywordKind.Hidden;
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func (parser : &mut Parser) not_id_val_err(prop : &std::string_view) {
    var errStr = std::string("unknown value for '")
    errStr.append_with_len(prop, prop.size())
    errStr.append('\'')
    const n = std::string_view(", property requires an identifier value")
    errStr.append_with_len(n.data(), n.size())
    parser.error(std::string_view(errStr.data(), errStr.size()))
}

func (parser : &mut Parser) wrong_val_kw_err(prop : &std::string_view) {
    var errStr = std::string("unknown value for '")
    errStr.append_with_len(prop, prop.size())
    errStr.append('\'');
    parser.error(std::string_view(errStr.data(), errStr.size()))
}

func (cssParser : &mut CSSParser) parseFontWeight(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        const kind = getFontWeightKeywordKind(token.value.data())
        if(kind == CSSKeywordKind.Unknown) {
            parser.wrong_val_kw_err("font-weight")
        }
        parser.increment();
        alloc_value_keyword(builder, value, kind, token.value)
    } else if(token.type == TokenType.Number) {
        parser.increment();
        alloc_value_number(builder, value, token.value);
    } else {
        parser.wrong_val_kw_err("font-weight");
        return;
    }
}

func (cssParser : &mut CSSParser) parseFontSize(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        const kind = getFontSizeKeywordKind(token.value.data())
        if(kind == CSSKeywordKind.Unknown) {
            parser.wrong_val_kw_err("font-size");
        }
        parser.increment();
        alloc_value_keyword(builder, value, kind, token.value)
    } else if(token.type == TokenType.Number) {
        parser.increment();
        alloc_value_length(parser, builder, value, token.value);
    } else {
        parser.wrong_val_kw_err("font-size");
        return;
    }
}

func (cssParser : &mut CSSParser) parseTextAlign(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-align");
        return;
    }
    const kind = getTextAlignKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-align");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseDisplay(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("display");
        return;
    }
    const kind = getDisplayKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("display");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parsePosition(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("position");
        return;
    }
    const kind = getPositionKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("position");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseOverflow(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("overflow");
        return;
    }
    const kind = getOverflowKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("overflow");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFloat(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("float");
        return;
    }
    const kind = getFloatKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("float");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseClear(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("clear");
        return;
    }
    const kind = getClearKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("clear");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseVerticalAlign(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("vertical-align");
        return;
    }
    const kind = getVerticalAlignKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("vertical-align");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseWhitespace(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("white-space");
        return;
    }
    const kind = getWhitespaceKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("white-space");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTextTransform(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-transform");
        return;
    }
    const kind = getTextTransformKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-transform");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseVisibility(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("visibility");
        return;
    }
    const kind = getVisibilityKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("visibility");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseCursor(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("cursor");
        return;
    }
    const kind = getCursorKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("cursor");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseDirection(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("direction");
        return;
    }
    const kind = getDirectionKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("direction");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseResize(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("resize");
        return;
    }
    const kind = getResizeKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("resize");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTableLayout(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("table-layout");
        return;
    }
    const kind = getTableLayoutKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("table-layout");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBorderCollapse(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("border-collapse");
        return;
    }
    const kind = getBorderCollapseKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("border-collapse");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseTextOverflow(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("text-overflow");
        return;
    }
    const kind = getTextOverflowKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("text-overflow");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseOverflowWrap(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("overflow-wrap");
        return;
    }
    const kind = getOverflowWrapKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("overflow-wrap");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseWordBreak(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("word-break");
        return;
    }
    const kind = getWordBreakKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("word-break");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseObjectFit(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("object-fit");
        return;
    }
    const kind = getObjectFitKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("object-fit");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseImageRendering(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("image-rendering");
        return;
    }
    const kind = getImageRenderingKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("image-rendering");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBackFaceVisibilityValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("backface-visibility");
        return;
    }
    const kind = getBackFaceVisibilityKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("backface-visibility");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFontStyleValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-style");
        return;
    }
    const kind = getFontStyleKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-style");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseFontVariantValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("font-variant");
        return;
    }
    const kind = getFontVariantKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("font-variant");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseListStyleType(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("list-style-type");
        return;
    }
    const kind = getListStyleTypeKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("list-style-type");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseListStylePosition(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("list-style-position");
        return;
    }
    const kind = getListStylePositionKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("list-style-position");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseAlignItems(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.not_id_val_err("align-items");
        return;
    }
    const kind = getAlignItemsKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.wrong_val_kw_err("align-items");
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}