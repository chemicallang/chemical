import "./Position.ch"
import "@std/string_view.ch"

struct Token {

    var type : int

    var value : std::string_view

    var position : Position


}