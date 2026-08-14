// Regression tests: JS statements must be emitted into the generated client JS,
// not silently dropped by the converter. Previously switch/while/do-while/throw/
// break/continue/for-of/class had no emit case in convertJsNode, so the statements
// vanished from the output.

#universal SwitchEmit(props) {
    var label = "none"
    switch(props.kind) {
        case "a": label = "A"
        break
        case "b": label = "B"
        break
        default: label = "D"
    }
    return <span>{label}</span>
}

@test
public func universal_switch_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <SwitchEmit kind="a" /> }
    var js = page.getJs()
    if(js.contains("switch(") && js.contains("case ") && js.contains("default:")) {
        env.success("switch statement is emitted")
    } else {
        env.error("switch statement was dropped")
        env.info(js.data())
    }
}

#universal WhileEmit(props) {
    var sum = 0
    var i = 0
    while(i < props.n) {
        sum += i
        i++
    }
    return <span>{sum}</span>
}

@test
public func universal_while_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <WhileEmit n={5} /> }
    var js = page.getJs()
    if(js.contains("while(") && js.contains("sum += i")) {
        env.success("while loop is emitted")
    } else {
        env.error("while loop was dropped")
        env.info(js.data())
    }
}

#universal DoWhileEmit(props) {
    var sum = 0
    var i = 0
    do {
        sum += i
        i++
    } while(i < props.n)
    return <span>{sum}</span>
}

@test
public func universal_do_while_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <DoWhileEmit n={5} /> }
    var js = page.getJs()
    if(js.contains("do ") && js.contains("while(")) {
        env.success("do-while loop is emitted")
    } else {
        env.error("do-while loop was dropped")
        env.info(js.data())
    }
}

#universal ThrowEmit(props) {
    if(props.bad) {
        throw "boom"
    }
    return <span>ok</span>
}

@test
public func universal_throw_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ThrowEmit /> }
    var js = page.getJs()
    if(js.contains("throw ")) {
        env.success("throw statement is emitted")
    } else {
        env.error("throw statement was dropped")
        env.info(js.data())
    }
}

#universal BreakContinueEmit(props) {
    var sum = 0
    for(var i = 0; i < 10; i++) {
        if(i == 5) break
        if(i == 3) continue
        sum += i
    }
    return <span>{sum}</span>
}

@test
public func universal_break_continue_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <BreakContinueEmit /> }
    var js = page.getJs()
    if(js.contains("break;") && js.contains("continue;")) {
        env.success("break and continue are emitted")
    } else {
        env.error("break/continue were dropped")
        env.info(js.data())
    }
}

#universal ForOfEmit(props) {
    var out = ""
    for(var item of props.items) {
        out += item
    }
    return <span>{out}</span>
}

@test
public func universal_for_of_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ForOfEmit /> }
    var js = page.getJs()
    if(js.contains("for(") && js.contains(" of ")) {
        env.success("for-of loop is emitted")
    } else {
        env.error("for-of loop was dropped")
        env.info(js.data())
    }
}

#universal ClassDeclEmit(props) {
    class Point {
        constructor(x, y) {
            this.x = x
            this.y = y
        }
    }
    var p = new Point(1, 2)
    return <span>{p.x}</span>
}

@test
public func universal_class_decl_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ClassDeclEmit /> }
    var js = page.getJs()
    if(js.contains("class Point") && js.contains("constructor")) {
        env.success("class declaration is emitted")
    } else {
        env.error("class declaration was dropped")
        env.info(js.data())
    }
}

// Modulo operator
#universal ModuloEmit(props) {
    var rem = props.n % 3
    return <span>{rem}</span>
}

@test
public func universal_modulo_emitted_js(env : &mut TestEnv) {
    var page = HtmlPage()
    #html { <ModuloEmit n={7} /> }
    var js = page.getJs()
    if(js.contains("%")) {
        env.success("modulo operator is emitted")
    } else {
        env.error("modulo operator failed to parse/emit")
        env.info(js.data())
    }
}
