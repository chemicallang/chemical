import "/ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "/lexer/TokenType.ch"
import "/parser/value/length.ch"
import "/parser/CSSParser.ch"
import "/ast/CSSColorKind.ch"
import "/ast/CSSKeywordKind.ch"
import "/utils/color_utils.ch"

func (cssParser : &mut CSSParser) parseSingleMarginValue(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) : bool {

    const token = parser.getToken();
    switch(token.type) {
        TokenType.Number => {
            parser.increment();
            var number_value = builder.allocate<CSSLengthValueData>()
            new (number_value) CSSLengthValueData {
                kind : CSSLengthKind.Unknown,
                value : builder.allocate_view(token.value)
            }
            value.kind = CSSValueKind.Length
            number_value.kind = parseLengthKind(parser, builder);
            value.data = number_value
            return true;
        }
        TokenType.Identifier => {
            if(token.value.equals("auto")) {
                parser.increment();
                var kw_value = builder.allocate<CSSKeywordValueData>()
                new (kw_value) CSSKeywordValueData {
                    kind : CSSKeywordKind.Auto,
                    value : builder.allocate_view(token.value)
                }
                value.kind = CSSValueKind.Keyword
                value.data = kw_value
                return true;
            } else {
                return false;
            }
        }
        default => {
            return false;
        }
    }

}

func (cssParser : &mut CSSParser) parseMargin(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    if(cssParser.parseSingleMarginValue(parser, builder, value)) {

        const tok = parser.getToken();
        if(tok.type == TokenType.Semicolon) {
            return;
        }

        var multiple = builder.allocate<CSSMultipleValues>()
        new (multiple) CSSMultipleValues {
            values : std::vector<CSSValue>()
        }

        multiple.values.push(*value)
        value.kind = CSSValueKind.Multiple
        value.data = multiple

        var i = 0;
        while(i < 4) {
            var next : CSSValue
            if(cssParser.parseSingleMarginValue(parser, builder, next)) {

                multiple.values.push(next)
                i++;

            } else {
                const next = parser.getToken();
                if(next.type != TokenType.Semicolon) {
                    parser.error("unknown value for margin");
                }
                break;
            }
        }

    } else {
        parser.error("unknown value for margin");
    }

}