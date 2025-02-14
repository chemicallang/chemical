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

const fontWeightValueErr = "unknown value for font-weight"
const fontSizeValueErr = "unknown value for font-size"
const textAlignValueErr = "unknown value for text-align"
const displayValueErr = "unknown value for display"
const positionValueErr = "unknown value for position"
const overflowValueErr = "unknown value for overflow"
const floatValueErr = "unknown value for float"
const clearValueErr = "unknown value for clear"
const verticalAlignValueErr = "unknown value for vertical-align"
const whitespaceValueErr = "unknown value for white-space"
const textTransformValueErr = "unknown value for text-transform"
const visibilityValueErr = "unknown value for visibility"
const cursorValueErr = "unknown value for cursor"
const directionValueErr = "unknown value for direction"
const resizeValueErr = "unknown value for resize"
const tableLayoutValueErr = "unknown value for table-layout"
const borderCollapseValueErr = "unknown value for border-collapse"
const textOverflowValueErr = "unknown value for text-overflow"
const overflowWrapValueErr = "unknown value for overflow-wrap"
const wordBreakValueErr = "unknown value for word-break"
const objectFitValueErr = "unknown value for object-fit"
const imageRenderingValueErr = "unknown value for image-rendering"
const backFaceVisibilityValueErr = "unknown value for backface-visibility"

func (cssParser : &mut CSSParser) parseFontWeight(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        const kind = getFontWeightKeywordKind(token.value.data())
        if(kind == CSSKeywordKind.Unknown) {
            parser.error(fontWeightValueErr);
        }
        parser.increment();
        alloc_value_keyword(builder, value, kind, token.value)
    } else if(token.type == TokenType.Number) {
        parser.increment();
        alloc_value_number(builder, value, token.value);
    } else {
        parser.error(fontWeightValueErr);
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
            parser.error(fontSizeValueErr);
        }
        parser.increment();
        alloc_value_keyword(builder, value, kind, token.value)
    } else if(token.type == TokenType.Number) {
        parser.increment();
        alloc_value_length(parser, builder, value, token.value);
    } else {
        parser.error(fontSizeValueErr);
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
        parser.error(textAlignValueErr);
        return;
    }
    const kind = getTextAlignKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(textAlignValueErr);
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
        parser.error(displayValueErr);
        return;
    }
    const kind = getDisplayKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(displayValueErr);
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
        parser.error(positionValueErr);
        return;
    }
    const kind = getPositionKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(positionValueErr);
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
        parser.error(overflowValueErr);
        return;
    }
    const kind = getOverflowKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(overflowValueErr);
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
        parser.error(floatValueErr);
        return;
    }
    const kind = getFloatKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(floatValueErr);
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
        parser.error(clearValueErr);
        return;
    }
    const kind = getClearKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(clearValueErr);
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
        parser.error(verticalAlignValueErr);
        return;
    }
    const kind = getVerticalAlignKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(verticalAlignValueErr);
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
        parser.error(whitespaceValueErr);
        return;
    }
    const kind = getWhitespaceKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(whitespaceValueErr);
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
        parser.error(textTransformValueErr);
        return;
    }
    const kind = getTextTransformKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(textTransformValueErr);
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
        parser.error(visibilityValueErr);
        return;
    }
    const kind = getVisibilityKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(visibilityValueErr);
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
        parser.error(cursorValueErr);
        return;
    }
    const kind = getCursorKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(cursorValueErr);
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
        parser.error(directionValueErr);
        return;
    }
    const kind = getDirectionKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(directionValueErr);
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
        parser.error(resizeValueErr);
        return;
    }
    const kind = getResizeKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(resizeValueErr);
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
        parser.error(tableLayoutValueErr);
        return;
    }
    const kind = getTableLayoutKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(tableLayoutValueErr);
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
        parser.error(borderCollapseValueErr);
        return;
    }
    const kind = getBorderCollapseKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(borderCollapseValueErr);
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
        parser.error(textOverflowValueErr);
        return;
    }
    const kind = getTextOverflowKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(textOverflowValueErr);
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
        parser.error(textOverflowValueErr);
        return;
    }
    const kind = getOverflowWrapKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(overflowWrapValueErr);
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
        parser.error(wordBreakValueErr);
        return;
    }
    const kind = getWordBreakKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(wordBreakValueErr);
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
        parser.error(objectFitValueErr);
        return;
    }
    const kind = getObjectFitKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(objectFitValueErr);
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
        parser.error(imageRenderingValueErr);
        return;
    }
    const kind = getImageRenderingKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(imageRenderingValueErr);
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}

func (cssParser : &mut CSSParser) parseBackFaceVisibilityValueErr(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {
    const token = parser.getToken();
    if(token.type != TokenType.Identifier) {
        parser.error(backFaceVisibilityValueErr);
        return;
    }
    const kind = getBackFaceVisibilityKeywordKind(token.value.data())
    if(kind == CSSKeywordKind.Unknown) {
        parser.error(backFaceVisibilityValueErr);
    }
    parser.increment();
    alloc_value_keyword(builder, value, kind, token.value)
}