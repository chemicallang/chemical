
public struct Token {

    var type : int

    var value : std::string_view

    var position : Position

    if(def.lsp) {
        var linked : *mut ASTAny = null
    }

}