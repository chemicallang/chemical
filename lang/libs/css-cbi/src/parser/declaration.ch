import "../ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "../lexer/TokenType.ch"
import "@std/hashing/fnv1.ch"
import "@std/std.ch"
import "@std/string_view.ch"
import "./value/length.ch"
import "./CSSParser.ch"
import "/ast/CSSColorKind.ch"
import "/ast/CSSKeywordKind.ch"
import "/utils/color_utils.ch"
import "@compiler/Parser.ch"
import "@compiler/ASTBuilder.ch"
import "@cstd/stdio.ch"

func getCSSGlobalKeywordKind(ptr : *char) : CSSKeywordKind {
    switch(fnv1_hash(ptr)) {
        comptime_fnv1_hash("inherit") => {
            return CSSKeywordKind.Inherit;
        }
        comptime_fnv1_hash("initial") => {
            return CSSKeywordKind.Initial
        }
        comptime_fnv1_hash("unset") => {
            return CSSKeywordKind.Unset
        }
        default => {
            return CSSKeywordKind.Unknown
        }
    }
}

func (cssParser : &mut CSSParser) parseRandomValue(parser : *mut Parser, builder : *mut ASTBuilder, value : &mut CSSValue) {
    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            alloc_value_length(parser, builder, value, token.value);
            return
        }
        TokenType.Identifier => {
            return cssParser.parseIdentifierCSSColor(parser, builder, value, token)
        }
        TokenType.HexColor => {
            cssParser.parseHexColor(parser, builder, token.value, value);
            parser.increment();
            return;
        }
        default => {
            parser.error("unknown value token with data");
        }
    }
}

func (cssParser : &mut CSSParser) parseValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    propertyName : &std::string_view
) {

    const valueTok = parser.getToken();
    if(valueTok.type == TokenType.Identifier) {
        const globalKind = getCSSGlobalKeywordKind(valueTok.value.data());
        if(globalKind != CSSKeywordKind.Unknown) {
            parser.increment();
            var kw_value = builder.allocate<CSSKeywordValueData>();
            new (kw_value) CSSKeywordValueData {
                kind : globalKind,
                value : builder.allocate_view(valueTok.value)
            }
            value.kind = CSSValueKind.Keyword;
            value.data = kw_value;
            return;
        }
    }

    const parserFn = cssParser.getParserFor(propertyName);

    if(parserFn == null) {

        cssParser.parseRandomValue(parser, builder, value);

    } else {

        parserFn(cssParser, parser, builder, value);

    }

}

func (cssParser : &mut CSSParser) parseDeclaration(parser : *mut Parser, builder : *mut ASTBuilder) : *mut CSSDeclaration {

    printf("css parsing a declaration\n");
    fflush(null)

    const token = parser.getToken();
    if(token.type == TokenType.Identifier) {
        parser.increment();
    } else if(token.type == TokenType.Comment) {
        parser.increment();
        return cssParser.parseDeclaration(parser, builder)
    } else {
        return null;
    }

    const decl = builder.allocate<CSSDeclaration>();
    new (decl) CSSDeclaration {
        property : CSSProperty {
            kind : CSSPropertyKind.Unknown,
            name : builder.allocate_view(token.value)
        },
        value : CSSValue {
            kind : CSSValueKind.Unknown,
            data : null
        }
    }

    const col = parser.getToken();
    if(col.type == TokenType.Colon) {
        parser.increment();
    } else {
        parser.error("expected colon after the css property name");
    }

    printf("css parsing value\n");
    fflush(null)

    cssParser.parseValue(parser, builder, decl.value, token.value);

    const sc = parser.getToken();
    if(sc.type == TokenType.Semicolon) {
        parser.increment();
    } else {
        parser.error("expected a semicolon after the property's value");
    }

    printf("parsed a declaration with key '%s'\n", token.value.data());
    fflush(null)

    return decl;

}