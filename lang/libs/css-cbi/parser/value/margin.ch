import "/ast/CSSDeclaration.ch"
import "@compiler/Token.ch"
import "/lexer/TokenType.ch"
import "/parser/value/length.ch"
import "/parser/CSSParser.ch"
import "/ast/CSSColorKind.ch"
import "/ast/CSSKeywordKind.ch"
import "/utils/color_utils.ch"

func (cssParser : &mut CSSParser) parseMargin(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

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
            return
        }
        TokenType.Identifier => {
            parser.increment();
            if(token.value.equals("auto")) {
                var kw_value = builder.allocate<CSSKeywordValueData>()
                new (kw_value) CSSKeywordValueData {
                    kind : CSSKeywordKind.Auto,
                    value : builder.allocate_view(token.value)
                }
                value.kind = CSSValueKind.Keyword
                value.data = kw_value
            } else {
                parser.error("unsupported identifier value for margin");
            }
        }
        default => {
            parser.error("unknown value token with data");
        }
    }

}