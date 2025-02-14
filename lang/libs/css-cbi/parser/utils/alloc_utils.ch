import "../ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "../lexer/TokenType.ch"
import "@std/hashing/fnv1.ch"
import "./value/length.ch"
import "./CSSParser.ch"
import "/ast/CSSColorKind.ch"
import "/ast/CSSKeywordKind.ch"
import "/utils/color_utils.ch"

func alloc_value_keyword(
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    kind : CSSKeywordKind,
    view : &std::string_view
) {
    var kw_value = builder.allocate<CSSKeywordValueData>();
    new (kw_value) CSSKeywordValueData {
        kind : kind,
        value : builder.allocate_view(view)
    }
    value.kind = CSSValueKind.Keyword;
    value.data = kw_value;
}

func alloc_value_number(
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    view : &std::string_view
) {
    var number_value = builder.allocate<CSSLengthValueData>()
    new (number_value) CSSLengthValueData {
        kind : CSSLengthKind.None,
        value : builder.allocate_view(view)
    }
    value.kind = CSSValueKind.Length
    value.data = number_value
}

func alloc_value_length(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    view : &std::string_view
) {
    var number_value = builder.allocate<CSSLengthValueData>()
    new (number_value) CSSLengthValueData {
        kind : CSSLengthKind.Unknown,
        value : builder.allocate_view(view)
    }
    number_value.kind = parseLengthKind(parser, builder);
    value.kind = CSSValueKind.Length
    value.data = number_value
}