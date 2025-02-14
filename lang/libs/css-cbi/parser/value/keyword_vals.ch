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

const fontWeightValueErr = "unknown value for font weight"
const fontSizeValueErr = "unknown value for font size"
const textAlignValueErr = "unknown value for text align"
const displayValueErr = "unknown value for display"
const positionValueErr = "unknown value for position"
const overflowValueErr = "unknown value for overflow"

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