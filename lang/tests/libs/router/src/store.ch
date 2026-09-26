using namespace std;

// ---------------------------------------------------------------------------
// Universal router parameter-store tests (Phase 1)
// ---------------------------------------------------------------------------

@test
public func test_router_parameter_text_roundtrip(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_parameter("greeting", "hello")
    if(!page.has_parameter("greeting")) {
        env.error("has_parameter should be true for an inserted key")
        return
    }
    const v = page.get_parameter("greeting")
    if(!v.equals(std::string_view("hello"))) {
        env.error("get_parameter should return the inserted value")
    }
}

@test
public func test_router_parameter_object_roundtrip(env : &mut TestEnv) {
    var page = HtmlPage()
    var value : int = 42
    page.add_parameter_object("ptr", (&raw mut value) as *mut void)
    const p = get_parameter_object<int>(&mut page, "ptr")
    if(p == null) {
        env.error("get_parameter_object should resolve an inserted object")
        return
    }
    if(*p != 42) {
        env.error("object value should round-trip")
    }
}

@test
public func test_router_parameter_miss_paths(env : &mut TestEnv) {
    var page = HtmlPage()
    if(page.has_parameter("nope")) {
        env.error("has_parameter on a miss should be false")
    }
    const v = page.get_parameter("nope")
    if(v.size() != 0) {
        env.error("get_parameter on a miss should be empty")
    }
    const p = get_parameter_object<int>(&mut page, "nope")
    if(p != null) {
        env.error("get_parameter_object on a miss should be null")
    }
}

@test
public func test_router_parameter_wrong_type(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_parameter("text", "x")
    const p = get_parameter_object<int>(&mut page, "text")
    if(p != null) {
        env.error("get_parameter_object on a Text parameter should be null")
    }
}

@test
public func test_router_route_missing_flag(env : &mut TestEnv) {
    var page = HtmlPage()
    if(page.route_missing()) {
        env.error("route_missing should default to false")
    }
    page.mark_route_missing()
    if(!page.route_missing()) {
        env.error("mark_route_missing should set the flag")
    }
}

@test
public func test_router_manifest_json(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_route_pattern("main", "/projects/{id}", "projects", false)
    page.add_route_pattern("main", "", "not-found", true)
    var json = page.route_manifest_json()
    var expected = std::string_view("{\"routers\":[{\"name\":\"main\",\"routes\":[{\"id\":\"projects\",\"pattern\":\"/projects/{id}\",\"fallback\":false},{\"id\":\"not-found\",\"pattern\":null,\"fallback\":true}]}]}")
    if(!json.to_view().equals(&expected)) {
        env.error("route_manifest_json did not match")
        env.info(json.data())
    }
}

@test
public func test_router_manifest_document(env : &mut TestEnv) {
    var page = HtmlPage()
    page.add_route_pattern("main", "/projects/{id}", "projects", false)
    page.add_route_pattern("main", "", "not-found", true)
    var json = page.route_manifest_document("index", "/app")
    var expected = std::string_view("{\"name\":\"index\",\"base\":\"/app\",\"routers\":[{\"name\":\"main\",\"routes\":[{\"id\":\"projects\",\"pattern\":\"/projects/{id}\",\"fallback\":false},{\"id\":\"not-found\",\"pattern\":null,\"fallback\":true}]}]}")
    if(!json.to_view().equals(&expected)) {
        env.error("route_manifest_document did not match template 6")
        env.info(json.data())
    }
}
