@test
public func simple_var_decl_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = 10
    }
    string_equals(env, page.toStringJsOnly(), "var x = 10;");
}

@test
public func if_decl_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = 0
        if(x) {
            x = 3
        } else {
            x = 5
        }
    }
    string_equals(env, page.toStringJsOnly(), "var x = 0;if(x){x = 3} else {x = 5}");
}

@test
public func function_decl_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        function give(thing) {
            var x = 0;
            return x;
        }
    }
    string_equals(env, page.toStringJsOnly(), "function give(thing){var x = 0;return x;}");
}

@test
public func expression_and_calls_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        if(window != null) {
            log("hello")
        }
    }
    string_equals(env, page.toStringJsOnly(), "if(window != null){log(\"hello\")}");
}