// P0.3a scaffold test. It proves the package graph imports/links and each
// package exposes only its declared scaffold marker. P0.2 replaces this with
// contract snippets; P0.3b adds the real headless test core.

func expect_scaffold(name : *char, value : u32) : bool {
    if(value == API_VERSION) { return true }
    unsafe { printf("ui scaffold FAIL %s value=%u expected=%u\n", name, value, API_VERSION) }
    return false
}

public func main() : int {
    var ok = true
    if(!expect_scaffold("ui", __ui_scaffold())) { ok = false }
    if(!expect_scaffold("ui_kit", __ui_scaffold_kit())) { ok = false }
    if(!expect_scaffold("ui_host", __ui_scaffold_host())) { ok = false }
    if(!expect_scaffold("ui_native", __ui_scaffold_native())) { ok = false }
    if(!expect_scaffold("ui_web", __ui_scaffold_web())) { ok = false }
    if(!expect_scaffold("ui_debug", __ui_scaffold_debug())) { ok = false }
    if(!expect_scaffold("ui_test", __ui_scaffold_test())) { ok = false }
    if(!expect_scaffold("ui_icons", __ui_scaffold_icons())) { ok = false }
    if(!expect_scaffold("ui_cli", __ui_scaffold_cli())) { ok = false }
    if(!expect_scaffold("ui_fonts", __ui_scaffold_fonts())) { ok = false }
    if(!expect_scaffold("ui_gpu", __ui_scaffold_gpu())) { ok = false }
    if(!expect_scaffold("ui_lint", __ui_scaffold_lint())) { ok = false }
    if(!ok) { return 1 }
    unsafe { write(1, "ui scaffold PASS\n" as *void, 17 as ulong) }
    return 0
}
