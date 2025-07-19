import "/ast/CSSDeclaration.ch"
import "/lexer/TokenType.ch"
import "/parser/value/length.ch"
import "/parser/CSSParser.ch"
import "/ast/CSSColorKind.ch"
import "/ast/CSSKeywordKind.ch"
import "/utils/color_utils.ch"

func (cssParser : &mut CSSParser) parsePadding(
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) {

    if(cssParser.parseLength(parser, builder, value)) {

        const tok = parser.getToken();
        if(tok.type == TokenType.Semicolon) {
            return;
        }

        var multiple = builder.allocate<CSSMultipleValues>()
        new (multiple) CSSMultipleValues {
            values : std::vector<CSSValue>()
        }

        multiple.values.push(value)
        value.kind = CSSValueKind.Multiple
        value.data = multiple

        var i = 0;
        while(i < 4) {
            var next : CSSValue
            if(cssParser.parseLength(parser, builder, next)) {

                multiple.values.push(next)
                i++;

            } else {
                const next = parser.getToken();
                if(next.type != TokenType.Semicolon) {
                    parser.error("unknown value for padding");
                }
                break;
            }
        }

    } else {
        parser.error("unknown value for padding");
    }

}