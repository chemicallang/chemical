using namespace std;

// ---------------------------------------------------------------------------
// Static-route SSR snapshot cache tests (Phase 7, §7.5).
//
// A route whose body is a purely-static native subtree renders identically on
// every request; the second render must reuse the cached bytes and produce
// byte-identical output.
// ---------------------------------------------------------------------------

#universal SnapshotApp(props) {
    router "ut-snap" {
        route default #"about" { <div class="about"><h1>About</h1><p>Static.</p></div> }
        route #"contact" { <div class="contact"><h1>Contact</h1></div> }
    }
}

#universal SnapshotDynPane(props) {
    return <div class="live">{props.text}</div>
}

#universal DynamicSnapshotApp(props) {
    router "ut-dynsnap" {
        route default #"live" { <SnapshotDynPane text="Dynamic" /> }
        route #"info" { <SnapshotDynPane text="Info" /> }
    }
}

func render_snapshot_app(page : &mut HtmlPage) {
    #html { <SnapshotApp /> }
}

func render_dynamic_snapshot_app(page : &mut HtmlPage) {
    #html { <DynamicSnapshotApp /> }
}

@test
public func test_router_snapshot_cold_then_warm(env : &mut TestEnv) {
    var p1 = HtmlPage()
    p1.reset_route_snapshots()
    render_snapshot_app(&mut p1)
    var cold = p1.getHtml()
    const misses = p1.route_snapshot_misses_count()

    var p2 = HtmlPage()
    render_snapshot_app(&mut p2)
    var warm = p2.getHtml()
    const hits = p2.route_snapshot_hits_count()

    if(misses == 0) {
        env.error("a cold render should populate route snapshots")
    }
    if(hits == 0) {
        env.error("a warm render should hit route snapshots")
    }
    if(cold.size() != warm.size() || !cold.equals(&warm)) {
        env.error("cold and warm renders must be byte-identical")
    }
}

@test
public func test_router_snapshot_ignores_component_routes(env : &mut TestEnv) {
    // A route whose body renders a component is dynamic: it must never be
    // snapshotted (the component's SSR may depend on request state).
    var page = HtmlPage()
    var before = page.route_snapshot_misses_count()
    render_dynamic_snapshot_app(&mut page)
    var after = page.route_snapshot_misses_count()
    if(after != before) {
        env.error("component-bodied routes must not be snapshotted")
    }
}

func snapshot_html() : std::string {
    var page = HtmlPage()
    render_snapshot_app(&mut page)
    const h = page.getHtml()
    return std::string(h.data(), h.size())
}

@test
public func test_router_snapshot_concurrent_render(env : &mut TestEnv) {
    // INV-17: 8 concurrent renders must produce byte-identical output.
    var expected = snapshot_html()
    var f1 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f2 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f3 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f4 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f5 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f6 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f7 = async::spawn_blocking<std::string>(() => snapshot_html())
    var f8 = async::spawn_blocking<std::string>(() => snapshot_html())
    var r1 = async::block_on(f1)
    var r2 = async::block_on(f2)
    var r3 = async::block_on(f3)
    var r4 = async::block_on(f4)
    var r5 = async::block_on(f5)
    var r6 = async::block_on(f6)
    var r7 = async::block_on(f7)
    var r8 = async::block_on(f8)
    if(!r1.equals(&expected) || !r2.equals(&expected) || !r3.equals(&expected) || !r4.equals(&expected) ||
       !r5.equals(&expected) || !r6.equals(&expected) || !r7.equals(&expected) || !r8.equals(&expected)) {
        env.error("concurrent snapshot renders must be byte-identical to the cold render")
    }
}
