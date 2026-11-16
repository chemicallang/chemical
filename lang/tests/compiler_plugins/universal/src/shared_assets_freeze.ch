// =============================================================================
// Shared bundle freeze + content-hashed names.
//
// The bundle is served with a long-lived cache, so its URL must change when its
// bytes do. `freeze()` records the content hash; rendering another page
// afterwards makes the sink stale, and a stale sink refuses to hand out a name —
// that is what stops a build from publishing a filename that disagrees with what
// is inside it.
// =============================================================================

// A component that only appears in the late page below, so rendering it appends
// to a sink that has already been frozen.
#universal SharedLateBadge(props) {
    return <span class="late">{props.children}</span>
}

func shared_build_late(page : &mut HtmlPage) {
    #html { <SharedLateBadge>x</SharedLateBadge> }
}

@test
public func shared_assets_freeze_hash_and_stale(env : &mut TestEnv) {
    var handle = SharedAssetsHandle.make("app")
    if(!handle.is_valid()) {
        env.error("shared sink allocation failed")
        return
    }
    var shared = handle.get_mut()

    var p1 = HtmlPage()
    p1.attach_shared_ref(shared)
    p1.defaultUniversalSetup()
    shared_build_greeting(&mut p1)

    var p2 = HtmlPage()
    p2.attach_shared_ref(shared)
    p2.defaultUniversalSetup()
    shared_build_greeting(&mut p2)

    // 1. the runtime and the component definition live exactly once, in the sink
    if(!shared.js().contains("$__uni_dispatch")) {
        env.error("shared bundle must carry the hydration runtime")
        return
    }
    if(shared_count(&shared.js(), &std::string_view("function universal_lib_test_Greeting(")) != 1) {
        env.error("component definition must appear exactly once in the shared bundle")
        return
    }
    // 2. a page's own JS never repeats the runtime or the definition
    var local2 = p2.local_js()
    if(local2.contains("$__uni_dispatch")) {
        env.error("a page must not repeat the hydration runtime")
        return
    }
    if(local2.contains("function universal_lib_test_Greeting(")) {
        env.error("a page must not repeat a component definition carried by the shared bundle")
        return
    }
    // 3. the page keeps its own dispatch
    if(!local2.contains("$__uni_c(")) {
        env.error("the page must keep its own dispatch statement")
        return
    }
    // 4. no name is offered before freeze
    if(shared.js_name().size() != 0) {
        env.error("a non-frozen sink must not offer a content-hashed name")
        return
    }
    // 5. freeze -> content-derived name carrying the sink's base name
    shared.freeze()
    var frozen_name = shared.js_name()
    if(frozen_name.size() == 0) {
        env.error("a frozen sink must offer a content-hashed name")
        return
    }
    var expected_prefix = std::string_view("app.")
    if(!frozen_name.starts_with(&expected_prefix)) {
        env.error("hashed name must carry the sink's base name")
        return
    }
    var js_suffix = std::string_view(".js")
    if(!frozen_name.ends_with(&js_suffix)) {
        env.error("hashed JS name must end in .js")
        return
    }
    if(shared.is_stale()) {
        env.error("a freshly frozen sink must not be stale")
        return
    }
    // 6. freezing again is deterministic (same bytes -> same name)
    shared.freeze()
    if(!shared.js_name().equals(&frozen_name)) {
        env.error("the content hash must be deterministic")
        return
    }
    // 7. rendering a page with a new component after freeze makes it stale, and a
    //    stale sink refuses to produce a name
    var p3 = HtmlPage()
    p3.attach_shared_ref(shared)
    p3.defaultUniversalSetup()
    shared_build_late(&mut p3)
    if(!shared.is_stale()) {
        env.error("appending after freeze must mark the sink stale")
        return
    }
    if(shared.js_name().size() != 0) {
        env.error("a stale sink must not offer a name that no longer matches its content")
        return
    }
    env.success("shared bundle freezes to a deterministic, hashed name and refuses to go stale")
}
