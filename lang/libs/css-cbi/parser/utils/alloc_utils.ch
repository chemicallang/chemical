import "@compiler/Token.ch"
import "@std/hashing/fnv1.ch"
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

func allocate_multiple_values(
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : *mut std::vector<CSSValue> {
    var multiple = builder.allocate<CSSMultipleValues>()
    new (multiple) CSSMultipleValues {
        values : std::vector<CSSValue>()
    }
    value.kind = CSSValueKind.Multiple
    value.data = multiple
    return &multiple.values
}

func alloc_two_value_keywords(
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    first_kind : CSSKeywordKind,
    second_kind : CSSKeywordKind,
    first_view : &std::string_view,
    second_view : &std::string_view
) {
    var first : CSSValue = CSSValue {
        kind : CSSValueKind.Keyword,
        data : null
    }
    var second : CSSValue = CSSValue {
        kind : CSSValueKind.Keyword,
        data : null
    }
    alloc_value_keyword(builder, first, first_kind, first_view)
    alloc_value_keyword(builder, second, second_kind, second_view)
    const values = allocate_multiple_values(builder, value)
    values.push(first)
    values.push(second)
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

func alloc_named_color(
    builder : *mut ASTBuilder,
    value : &mut CSSValue,
    view : &std::string_view
) {
    var col_value = builder.allocate<CSSColorValueData>();
    new (col_value) CSSColorValueData {
        kind : CSSColorKind.NamedColor,
        value : builder.allocate_view(view)
    }
    value.kind = CSSValueKind.Color
    value.data = col_value
}