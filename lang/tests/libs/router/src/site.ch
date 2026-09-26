using namespace std;

// ---------------------------------------------------------------------------
// Site-level static-export rewrite aggregate (design §13.2.3).
// ---------------------------------------------------------------------------

@test
public func test_router_site_routes_aggregate(env : &mut TestEnv) {
    var docs = std::vector<std::string_view>()
    docs.push(std::string_view("{\"name\":\"index\"}"))
    docs.push(std::string_view("{\"name\":\"about\"}"))
    const agg = site_routes_aggregate(&docs)
    const expected = std::string_view("[{\"name\":\"index\"},{\"name\":\"about\"}]")
    if(!agg.to_view().equals(&expected)) {
        env.error("site aggregate should wrap the per-page documents in a JSON array")
        env.info(agg.data())
    }
}

@test
public func test_router_write_site_routes(env : &mut TestEnv) {
    var dir = std::string(intrinsics::get_build_dir())
    dir.append_view(std::string_view("/site_routes_test"))
    fs::mkdir(dir.data())

    var p1 = std::string(dir.data(), dir.size())
    p1.append_view(std::string_view("/index.routes.json"))
    const d1 = std::string_view("{\"name\":\"index\",\"base\":\"\",\"routers\":[]}")
    fs::write_text_file(p1.data(), d1.data() as *u8, d1.size())

    var p2 = std::string(dir.data(), dir.size())
    p2.append_view(std::string_view("/about.routes.json"))
    const d2 = std::string_view("{\"name\":\"about\",\"base\":\"\",\"routers\":[]}")
    fs::write_text_file(p2.data(), d2.data() as *u8, d2.size())

    var names = std::vector<std::string_view>()
    names.push(std::string_view("index"))
    names.push(std::string_view("about"))
    names.push(std::string_view("missing"))
    const n = write_site_routes(dir.to_view(), &names)
    if(n != 2) {
        env.error("write_site_routes should aggregate the two present manifests")
    }

    var sitePath = std::string(dir.data(), dir.size())
    sitePath.append_view(std::string_view("/routes.json"))
    var rr = fs::read_entire_file(sitePath.data())
    if(rr is Result.Err) {
        env.error("routes.json should be written")
        return
    }
    var Ok(bytes) = rr else unreachable
    var content = std::string(bytes.data() as *char, bytes.size())
    const idx = std::string_view("{\"name\":\"index\"")
    if(content.find(&idx) == std::NPOS) {
        env.error("the aggregate should contain the index document")
        env.info(content.data())
    }
    const about = std::string_view("\"name\":\"about\"")
    if(content.find(&about) == std::NPOS) {
        env.error("the aggregate should contain the about document")
    }
}
