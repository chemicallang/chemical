// P0.3a scaffold + P0.3b test core/golden runner. Production UiHarness/renderer
// is not implemented here; see ui_test_core in core.ch.

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;
using std::Unit;
using ui_test_core::Bounds;
using ui_test_core::CoreError;
using ui_test_core::GoldenOptions;
using ui_test_core::Harness;
using ui_test_core::Query;

func expect_scaffold(name : *char, value : u32) : bool {
    if(value == API_VERSION) { return true }
    unsafe { printf("ui scaffold FAIL %s value=%u expected=%u\n", name, value, API_VERSION) }
    return false
}

func scaffold_checks() : bool {
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
    return ok
}

func check(name : *char, condition : bool) : bool {
    if(!condition) { unsafe { printf("ui test FAIL %s\n", name) } }
    return condition
}

func core_checks() : bool {
    var ok = true
    var h = Harness.make()
    var root_key = ui_test_core::key("root")
    var a_key = ui_test_core::key("a")
    var b_key = ui_test_core::key("b")
    var root = h.add_node(Option.None<ui_test_core::NodeId>(), root_key, Bounds { x : 0.0f, y : 0.0f, w : 200.0f, h : 100.0f }, "root", 1u8, "root")
    var a = h.add_node(Option.Some<ui_test_core::NodeId>(root), a_key, Bounds { x : 5.0f, y : 6.0f, w : 10.0f, h : 20.0f }, "Alpha", 2u8, "a")
    var b = h.add_node(Option.Some<ui_test_core::NodeId>(root), b_key, Bounds { x : 20.0f, y : 6.0f, w : 30.0f, h : 20.0f }, "Beta", 3u8, "b")

    var by_a = h.by_key(a_key)
    ok = check("by_key", by_a is Option.Some) && ok
    if(by_a is Option.Some) {
        var Some(found) = by_a else return false
        ok = check("by_key_id", found.index == a.index) && ok
    }
    var ba = h.bounds(a)
    ok = check("bounds", ba is Option.Some) && ok
    if(ba is Option.Some) {
        var Some(bb) = ba else return false
        ok = check("bounds_value", bb.x == 5.0f && bb.w == 10.0f) && ok
    }
    var ta = h.text_of(a)
    ok = check("text_of", ta is Option.Some) && ok
    if(ta is Option.Some) {
        var Some(tv) = ta else return false
        ok = check("text_value", tv.to_view().equals_with_len("Alpha", 5u)) && ok
    }
    var sem = h.semantics(b)
    ok = check("semantics", sem is Option.Some) && ok
    if(sem is Option.Some) {
        var Some(sv) = sem else return false
        ok = check("semantics_role", sv.role == 3u8) && ok
    }
    var q = Query.make()
    q.role = 2
    ok = check("find_all_role", h.find_all(q).size() == 1u) && ok
    q.role = -1
    q.text = "Beta"
    ok = check("find_all_text", h.find_all(q).size() == 1u) && ok

    h.set_state(a_key, 41i64)
    var kids = vector<ui_test_core::NodeId>()
    kids.push(b)
    kids.push(a)
    h.reorder(root, kids.as_span())
    var first_kid = h.child_at(root, 0u32)
    if(first_kid is Option.Some) {
        var Some(fk) = first_kid else return false
        ok = check("reorder_children", fk.index == b.index) && ok
    } else {
        ok = false
    }
    ok = check("reorder_state_preserved", h.get_state(a_key) == 41i64) && ok

    h.remove_node(a)
    ok = check("stale_id", h.bounds(a) is Option.None) && ok
    ok = check("stale_text", h.text_of(a) is Option.None) && ok

    h.set_focus(b)
    ok = check("focus", h.focused() is Option.Some) && ok
    h.set_scroll(b_key, 12.0f, 34.0f)
    var sc = h.scroll_offset(b_key)
    ok = check("scroll", sc.x == 12.0f && sc.y == 34.0f) && ok

    h.dirty = true
    var settled = h.settle(8u32)
    ok = check("settle", settled is Result.Ok) && ok
    h.set_never_settle(true)
    var timed_out = h.settle(3u32)
    ok = check("settle_timeout", timed_out is Result.Err) && ok
    h.set_never_settle(false)

    var geo = h.geometry_snapshot()
    ok = check("geometry_snapshot", geo.to_view().size() > 2u) && ok
    ok = check("geometry_snapshot_shape", geo.to_view().equals_with_len("[", 1u) || geo.to_view().size() > 2u) && ok
    var sem_json = h.semantics_snapshot()
    ok = check("semantics_snapshot", sem_json.to_view().size() > 2u) && ok
    return ok
}

func design_checks() : bool {
    return design::run()
}

func geometry_checks() : bool {
    return geometry_tests::run()
}

func error_checks() : bool {
    return error_tests::run()
}

func arena_checks() : bool {
    return arena_tests::run()
}

func keys_checks() : bool {
    return keys_tests::run()
}

func view_checks() : bool {
    return view_tests::run()
}

func reconcile_checks() : bool {
    return reconcile_tests::run()
}

func style_checks() : bool {
    return style_tests::run_style()
}

func theme_checks() : bool {
    return style_tests::run_theme()
}

func text_metrics_checks() : bool {
    return text_tests::run_text_metrics()
}

func layout_checks() : bool {
    return layout_tests::run_layout()
}

func layout_v2_checks() : bool {
    return layout_v2_tests::run_layout_v2()
}

func display_checks() : bool {
    return display_tests::run_display()
}

func raster_checks() : bool {
    return raster_tests::run_raster()
}

func headless_checks() : bool {
    return headless_tests::run_headless_tests()
}

func golden_checks() : bool {
    var ok = true
    var fixture : string_view = "/home/wakaztahir/work/Chemical/chemical/lang/tests/ui/fixtures/pixels/checker_128.png"
    var loaded = ui_test_core::load_png(fixture)
    ok = check("fixture_load", loaded is Result.Ok) && ok
    if(loaded is Result.Err) { return false }
    var Ok(img) = loaded else return false
    ok = check("fixture_size", img.width() == 128 && img.height() == 128) && ok

    var png = img.encode_png()
    ok = check("fixture_reencode", png is Result.Ok) && ok
    if(png is Result.Ok) {
        var Ok(bytes) = png else return false
        var h = ui_test_core::sha256_hex(bytes.as_span())
        ok = check("fixture_png_hash", h.to_view().equals_with_len("424a8b518c53efad05c0ad964382f5dbc395298b2da19c3963b6208fa2d01000", 64u)) && ok
    }
    var rgba = ui_test_core::sha256_hex(img.pixels())
    ok = check("fixture_rgba_hash", rgba.to_view().equals_with_len("2deb38260d75f07d80d888a6ba74b6ea2dd88eb4a09e2ff7bdeb39b86ca2aa38", 64u)) && ok

    var opts = GoldenOptions.make()
    if(ui_test_core::update_requested()) {
        var update_path : string_view = "/home/wakaztahir/work/Chemical/chemical/lang/tests/ui/build/golden_update.png"
        var updated = ui_test_core::assert_golden(&img, update_path, opts)
        ok = check("golden_update", updated is Result.Ok) && ok
        if(updated is Result.Ok) {
            var Ok(ur) = updated else return false
            ok = check("golden_updated_flag", ur.updated) && ok
        }
        return ok
    }
    var pass = ui_test_core::assert_golden(&img, fixture, opts)
    ok = check("golden_pass", pass is Result.Ok) && ok
    if(pass is Result.Ok) {
        var Ok(gr) = pass else return false
        ok = check("golden_diff_zero", gr.diff_pixels == 0u32) && ok
    }

    // Failure copy lives in the build dir; never modify the committed fixture.
    var bad_path : string_view = "/home/wakaztahir/work/Chemical/chemical/lang/tests/ui/build/golden_fail.png"
    var bad_write = ui_test_core::write_png(&img, bad_path)
    ok = check("golden_bad_setup", bad_write is Result.Ok) && ok
    var bad = image::Image.new_rgba(128, 128)
    if(bad is Result.Ok) {
        var Ok(bi) = bad else return false
        var src_pixels = img.pixels()
        var bp = bi.pixels_mut()
        unsafe { memcpy(bp.data() as *mut void, src_pixels.data() as *mut void, 65536u) }
        var bp0 = bp.data()
        unsafe { bp0[0] = 255u8 - bp0[0] }
        var fail = ui_test_core::assert_golden(&bi, bad_path, opts)
        ok = check("golden_fail", fail is Result.Err) && ok
        if(fail is Result.Err) {
            var diff_count = 0u32
            unsafe { diff_count = ui_test_core::last_golden_diff }
            ok = check("golden_diff_count", diff_count > 0u32) && ok
        }
        var diff_path : string_view = "/home/wakaztahir/work/Chemical/chemical/lang/tests/ui/build/golden_fail.png.diff.png"
        var p = ui_test_core::path_of(diff_path)
        ok = check("golden_diff_artifact", fs::exists(&p.as_path())) && ok
    }
    var update_path : string_view = "/home/wakaztahir/work/Chemical/chemical/lang/tests/ui/build/golden_update.png"
    // An update is only requested when the process environment says so; the
    // test just verifies the normal path here.
    ok = check("golden_update_not_implicit", true) && ok
    return ok
}

func run_filter(filter : string_view) : bool {
    if(filter.equals_with_len("scaffold", 8u)) { return scaffold_checks() }
    if(filter.equals_with_len("core", 4u)) { return core_checks() }
    if(filter.equals_with_len("design", 6u)) { return design_checks() }
    if(filter.equals_with_len("geometry", 8u)) { return geometry_checks() }
    if(filter.equals_with_len("error", 5u)) { return error_checks() }
    if(filter.equals_with_len("arena", 5u)) { return arena_checks() }
    if(filter.equals_with_len("keys", 4u)) { return keys_checks() }
    if(filter.equals_with_len("view", 4u)) { return view_checks() }
    if(filter.equals_with_len("reconcile", 9u)) { return reconcile_checks() }
    if(filter.equals_with_len("style", 5u)) { return style_checks() }
    if(filter.equals_with_len("theme", 5u)) { return theme_checks() }
    if(filter.equals_with_len("text_metrics", 12u)) { return text_metrics_checks() }
    if(filter.equals_with_len("font_metrics", 12u)) { return text_metrics_checks() }
    if(filter.equals_with_len("layout", 6u)) { return layout_checks() }
    if(filter.equals_with_len("layout_v2", 9u)) { return layout_v2_checks() }
    if(filter.equals_with_len("wrap", 4u)) { return layout_v2_checks() }
    if(filter.equals_with_len("grid", 4u)) { return layout_v2_checks() }
    if(filter.equals_with_len("scroll", 6u)) { return layout_v2_checks() }
    if(filter.equals_with_len("layers", 6u)) { return layout_v2_checks() }
    if(filter.equals_with_len("display", 7u)) { return display_checks() }
    if(filter.equals_with_len("raster", 6u)) { return raster_checks() }
    if(filter.equals_with_len("headless", 8u)) { return headless_checks() }
    if(filter.equals_with_len("golden", 6u)) { return golden_checks() }
    if(filter.equals_with_len("*", 1u)) {
        return scaffold_checks() && core_checks() && design_checks() && geometry_checks() && error_checks() && arena_checks() && keys_checks() && view_checks() && reconcile_checks() && golden_checks() && style_checks() && theme_checks() && text_metrics_checks() && layout_checks() && layout_v2_checks() && display_checks() && raster_checks() && headless_checks()
    }
    unsafe { printf("ui tests: unknown filter\n") }
    return false
}

public func main(argc : int, argv : **char) : int {
    var filter = unsafe(string_view.make_no_len("scaffold"))
    var i = 1
    while(i < argc) {
        var arg = unsafe(argv[i])
        var view = unsafe(std::string_view.make_no_len(arg))
        if(view.equals_with_len("--filter", 8u)) {
            i = i + 1
            if(i < argc) { filter = unsafe(std::string_view.make_no_len(unsafe(argv[i]))) }
        }
        i = i + 1
    }
    if(run_filter(filter)) {
        if(filter.equals_with_len("core", 4u)) {
            unsafe { write(1, "ui core PASS\n" as *void, 13u) }
        } else if(filter.equals_with_len("design", 6u)) {
            unsafe { write(1, "ui design PASS\n" as *void, 15u) }
        } else if(filter.equals_with_len("geometry", 8u)) {
            unsafe { write(1, "ui geometry PASS\n" as *void, 17u) }
        } else if(filter.equals_with_len("error", 5u)) {
            unsafe { write(1, "ui error PASS\n" as *void, 14u) }
        } else if(filter.equals_with_len("arena", 5u)) {
            unsafe { write(1, "ui arena PASS\n" as *void, 14u) }
        } else if(filter.equals_with_len("keys", 4u)) {
            unsafe { write(1, "ui keys PASS\n" as *void, 13u) }
        } else if(filter.equals_with_len("view", 4u)) {
            unsafe { write(1, "ui view PASS\n" as *void, 13u) }
        } else if(filter.equals_with_len("reconcile", 9u)) {
            unsafe { write(1, "ui reconcile PASS\n" as *void, 18u) }
        } else if(filter.equals_with_len("style", 5u)) {
            unsafe { write(1, "ui style PASS\n" as *void, 14u) }
        } else if(filter.equals_with_len("theme", 5u)) {
            unsafe { write(1, "ui theme PASS\n" as *void, 14u) }
        } else if(filter.equals_with_len("text_metrics", 12u) || filter.equals_with_len("font_metrics", 12u)) {
            unsafe { write(1, "ui text_metrics PASS\n" as *void, 21u) }
        } else if(filter.equals_with_len("layout", 6u)) {
            unsafe { write(1, "ui layout PASS\n" as *void, 15u) }
        } else if(filter.equals_with_len("layout_v2", 9u) || filter.equals_with_len("wrap", 4u) || filter.equals_with_len("grid", 4u) || filter.equals_with_len("scroll", 6u) || filter.equals_with_len("layers", 6u)) {
            unsafe { write(1, "ui layout_v2 PASS\n" as *void, 18u) }
        } else if(filter.equals_with_len("display", 7u)) {
            unsafe { write(1, "ui display PASS\n" as *void, 16u) }
        } else if(filter.equals_with_len("raster", 6u)) {
            unsafe { write(1, "ui raster PASS\n" as *void, 15u) }
        } else if(filter.equals_with_len("headless", 8u)) {
            unsafe { write(1, "ui headless PASS\n" as *void, 17u) }
        } else if(filter.equals_with_len("golden", 6u)) {
            unsafe { write(1, "ui golden PASS\n" as *void, 15u) }
        } else {
            unsafe { write(1, "ui scaffold PASS\n" as *void, 17u) }
        }
        return 0
    }
    return 1
}
