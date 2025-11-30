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
    string_equals(env, page.toStringJsOnly(), "var x = 0;if(x){x = 3;} else {x = 5;}");
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
    string_equals(env, page.toStringJsOnly(), "if(window != null){log(\"hello\");}");
}

@test
public func complex_chaining_calls_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        window.console.log("hello");
        window.document.getElementById("app").innerHTML = "Content";
    }
    string_equals(env, page.toStringJsOnly(), "window.console.log(\"hello\");window.document.getElementById(\"app\").innerHTML = \"Content\";");
}

@test
public func object_literal_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = { a : 10, b : "hello" }
    }
    string_equals(env, page.toStringJsOnly(), "var x = { a: 10, b: \"hello\" };");
}

@test
public func array_literal_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = [10, 20]
    }
    string_equals(env, page.toStringJsOnly(), "var x = [10, 20];");
}

@test
public func arrow_lambda_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = () => 10
    }
    string_equals(env, page.toStringJsOnly(), "var x = () => 10;");
}

@test
public func logical_operators_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        const a = true && false
    }
    string_equals(env, page.toStringJsOnly(), "const a = true && false;");
}

@test
public func not_operator_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        const c = !a
    }
    string_equals(env, page.toStringJsOnly(), "const c = !a;");
}

@test
public func ternary_operator_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        const c = 10 > 5 ? 10 : 5
    }
    string_equals(env, page.toStringJsOnly(), "const c = 10 > 5 ? 10 : 5;");
}

@test
public func unary_operator_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        const c = -5
    }
    string_equals(env, page.toStringJsOnly(), "const c = -5;");
}

@test
public func unary_operator_inc_dec_work(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        let count = 0;
        ++count;
        count++;
        --count;
        count--;
    }
    string_equals(env, page.toStringJsOnly(), "let count = 0;++count;count++;--count;count--;");
}

/**

@test
public func for_loop_break_continue(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        for(let i = 0; i < 10; i++) {
            if(i == 3) continue;
            if(i == 7) break;
            console.log(i);
        }
    }
    string_equals(env, page.toStringJsOnly(), "for(let i = 0; i < 10; i++) { if(i == 3) continue; if(i == 7) break; console.log(i); }");
}

@test
public func do_while_loop_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        let x = 0;
        do {
            x++
        } while(x < 3);
    }
    string_equals(env, page.toStringJsOnly(), "let x = 0; do { x++; } while(x < 3);");
}

@test
public func switch_stmt_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        const day = 2;
        switch(day) {
            case 1:
                console.log("Monday");
                break;
            case 2:
                console.log("Tuesday");
                break;
            default:
                console.log("Unknown");
        }
    }
    string_equals(env, page.toStringJsOnly(), """const day = 2; switch(day) {
case 1:
   console.log("Monday");
   break;
case 2:
   console.log("Tuesday");
   break;
default:
   console.log("Unknown");
}""");

}

@test
public func switch_stmt_works(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        try {
            throw "error";
        } catch(e) {
            console.log(e);
        } finally {
            console.log("done");
        }
    }
    string_equals(env, page.toStringJsOnly(), """try {
    throw "error";
} catch(e) {
    console.log(e);
} finally {
    console.log("done");
}""");

}

**/