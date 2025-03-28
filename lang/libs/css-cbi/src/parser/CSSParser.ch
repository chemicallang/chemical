import "utils/NamedColorMap.ch"
import "utils/ValueParserMap.ch"
import "@compiler/ASTBuilder.ch"
import "/ast/CSSDeclaration.ch"
import "@compiler/Parser.ch"
import "@std/string_view.ch"

type ValueParserFn = (
    cssParser : &mut CSSParser,
    parser : *mut Parser,
    builder : *mut ASTBuilder,
    value : &mut CSSValue
) => void

struct CSSParser {

    var colorMap : NamedColorMap

    var valueFnMap : ValueParserMap

    /**
     * if NOT has dynamic values, we will automatically put
     * class name that is hashed and prefixed with 'h'
     */
    var has_dynamic_values : bool = false

    func isColor(&self, color : &std::string_view) : bool {
        return colorMap.isColor(color);
    }

    func isNamedColor(&self, color : &std::string_view) : bool {
        return colorMap.isColor(color)
    }

    func getParserFor(&self, name : &std::string_view) : ValueParserFn {
        return valueFnMap.getParserFor(name) as ValueParserFn;
    }

}