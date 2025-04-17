import "@std/string_view.ch"

public struct Token {

    var type : int

    var value : std::string_view

    var position : Position

}