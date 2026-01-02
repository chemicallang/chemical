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
    string_equals(env, page.toStringJsOnly(), "for(let i = 0; i < 10; i++){if(i == 3)continue;if(i == 7)break;console.log(i);}");
}

@test
public func while_loop_break_continue(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        while(true) {
            if(i == 3) continue;
            if(i == 7) break;
            console.log(i);
        }
    }
    string_equals(env, page.toStringJsOnly(), "while(true){if(i == 3)continue;if(i == 7)break;console.log(i);}");
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
    string_equals(env, page.toStringJsOnly(), "let x = 0;do {x++;} while(x < 3);");
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
    string_equals(env, page.toStringJsOnly(), """const day = 2;switch(day) {case 1:console.log("Monday");break;case 2:console.log("Tuesday");break;default:console.log("Unknown");}""");

}

@test
public func try_catch_works(env : &mut TestEnv) {
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
    string_equals(env, page.toStringJsOnly(), """try {throw "error";} catch(e) {console.log(e);} finally {console.log("done");}""");

}

@test
public func test_literals(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var a = true;
        var b = false;
        var c = null;
        var d = undefined;
    }
    string_equals(env, page.toStringJsOnly(), """var a = true;var b = false;var c = null;var d = undefined;""");
}

@test
public func test_assignment_ops(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var x = 10;
        x += 5;
        x -= 2;
        x *= 3;
        x /= 2;
    }
    string_equals(env, page.toStringJsOnly(), """var x = 10;x += 5;x -= 2;x *= 3;x /= 2;""");
}

@test
public func test_unary_ops(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var t = typeof x;
        void 0;
        delete x.p;
        var b = ~x;
    }
    string_equals(env, page.toStringJsOnly(), """var t = typeof x;void 0;delete x.p;var b = ~x;""");
}

@test
public func test_binary_ops(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var a = x in y;
        var b = x instanceof y;
        var c = x & y;
        var d = x | y;
        var e = x ^ y;
        var f = x << 1;
        var g = x >> 1;
        var h = x >>> 1;
    }
    string_equals(env, page.toStringJsOnly(), """var a = x in y;var b = x instanceof y;var c = x & y;var d = x | y;var e = x ^ y;var f = x << 1;var g = x >> 1;var h = x >>> 1;""");
}

@test
public func test_new_expression(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var n = new Date();
        var m = new Array(1, 2);
    }
    string_equals(env, page.toStringJsOnly(), """var n = new Date();var m = new Array(1, 2);""");
}

@test
public func test_modern_loops(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        for(var k in obj) {
            console.log(k);
        }
        for(var v of arr) {
            console.log(v);
        }
    }
    string_equals(env, page.toStringJsOnly(), """for(var k in obj){console.log(k);}for(var v of arr){console.log(v);}""");
}

@test
public func test_template_literals(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var s = `hello world`;
        var m = `multi
line`;
    }
    string_equals(env, page.toStringJsOnly(), """var s = `hello world`;var m = `multi
line`;""");
}

@test
public func test_spread_operator(env : &mut TestEnv) {
    var page = HtmlPage()
    #js {
        var a = [1, ...b, 2];
        fn(...args);
    }
    string_equals(env, page.toStringJsOnly(), """var a = [1, ...b, 2];fn(...args);""");
}