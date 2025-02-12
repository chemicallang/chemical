import "/parser/CSSParser.ch"
import "@compiler/ASTBuilder.ch"
import "/ast/CSSDeclaration.ch"

// type ValueParserFn = (
//     cssParser : &mut CSSParser,
//     parser : *mut Parser,
//     builder : *mut ASTBuilder,
//     value : &mut CSSValue
// ) => void