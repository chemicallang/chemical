import "./Position.ch"
import "./TokenType.ch"
import "@std/string_view.ch"

struct Token {

    var type : TokenType

    var value : std::string_view

    var position : Position


}