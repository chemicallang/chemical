@test
public func simple_var_decl_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = 10
    }
    string_equals(env, page.toStringJsOnly(), "var x = 10;");
}