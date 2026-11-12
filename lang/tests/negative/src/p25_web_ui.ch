// P2.5 — the safe window/webview handles must never be Send.
//
// Both types own native UI-thread state; `window::Window` carries an
// explicit negative impl and `webview::WebView` is structurally !Send
// because its state pointer is raw. These two cases pin that at the type level.

// `NEG_MOD_WIN_LIBS` in main.ch does not import core, so `Send` is out of
// scope in the generated test program. This variant adds it.
internal const P25_WIN_LIBS = "application neg_test\nsource \".\"\nimport cstd\nimport core\nimport std\nimport test\nimport window\nimport webview\n"

@test @test.timeout(120000) public func neg_p25_window_handle_is_not_send(env : &mut TestEnv) {
    var ch = "public func main() : int {\n    var created = window::Window.new(\"w\")\n    if(created is std::Result.Ok) {\n        var Ok(w) = created else unsafe { unreachable }\n        return needs_send(w)\n    }\n    return 0\n}\nfunc <T> needs_send(v : T) : int where T : Send { return 0 }\n"
    expect_compile_error_with_mod(env, "p25_window_not_send", ch,
        "is declared !Send", P25_WIN_LIBS)
}

@test @test.timeout(120000) public func neg_p25_webview_handle_is_not_send(env : &mut TestEnv) {
    var ch = "public func main() : int {\n    var created = webview::WebView.new(\"w\")\n    if(created is std::Result.Ok) {\n        var Ok(w) = created else unsafe { unreachable }\n        return needs_send(w)\n    }\n    return 0\n}\nfunc <T> needs_send(v : T) : int where T : Send { return 0 }\n"
    expect_compile_error_with_mod(env, "p25_webview_not_send", ch,
        "does not satisfy where clause constraint 'T : Send'", P25_WIN_LIBS)
}
