using std::string;

// ---------------------------------------------------------------------------
// Mime type detection tests
// ---------------------------------------------------------------------------

@test
func test_mime_html(env : &mut TestEnv) {
    var ct = mime::get_type(".html")
    if(strcmp(ct.data(), "text/html") != 0) { env.error(".html should be text/html") }
}

@test
func test_mime_css(env : &mut TestEnv) {
    var ct = mime::get_type(".css")
    if(strcmp(ct.data(), "text/css") != 0) { env.error(".css should be text/css") }
}

@test
func test_mime_js(env : &mut TestEnv) {
    var ct = mime::get_type(".js")
    if(strcmp(ct.data(), "application/javascript") != 0) { env.error(".js should be application/javascript") }
}

@test
func test_mime_json(env : &mut TestEnv) {
    var ct = mime::get_type(".json")
    if(strcmp(ct.data(), "application/json") != 0) { env.error(".json should be application/json") }
}

@test
func test_mime_png(env : &mut TestEnv) {
    var ct = mime::get_type(".png")
    if(strcmp(ct.data(), "image/png") != 0) { env.error(".png should be image/png") }
}

@test
func test_mime_jpg(env : &mut TestEnv) {
    var ct = mime::get_type(".jpg")
    if(strcmp(ct.data(), "image/jpeg") != 0) { env.error(".jpg should be image/jpeg") }
}

@test
func test_mime_unknown(env : &mut TestEnv) {
    var ct = mime::get_type(".xyz")
    if(strcmp(ct.data(), "application/octet-stream") != 0) { env.error("unknown ext should be application/octet-stream") }
}

@test
func test_mime_wav(env : &mut TestEnv) {
    var ct = mime::get_type(".wav")
    if(strcmp(ct.data(), "audio/wav") != 0) { env.error(".wav should be audio/wav") }
}

@test
func test_mime_ttf(env : &mut TestEnv) {
    var ct = mime::get_type(".ttf")
    if(strcmp(ct.data(), "font/ttf") != 0) { env.error(".ttf should be font/ttf") }
}

@test
func test_mime_pdf(env : &mut TestEnv) {
    var ct = mime::get_type(".pdf")
    if(strcmp(ct.data(), "application/pdf") != 0) { env.error(".pdf should be application/pdf") }
}

// ---------------------------------------------------------------------------
// Extension extraction tests
// ---------------------------------------------------------------------------

@test
func test_mime_extension_basic(env : &mut TestEnv) {
    var ext = mime::extension("/foo/bar.css")
    if(!ext.equals_view(std.string_view(".css"))) { env.error("/foo/bar.css should give .css") }
}

@test
func test_mime_extension_no_ext(env : &mut TestEnv) {
    var ext = mime::extension("/foo/bar")
    if(ext.size() != 0) { env.error("/foo/bar should give empty ext") }
}

@test
func test_mime_extension_deep_path(env : &mut TestEnv) {
    var ext = mime::extension("/a/b/c/test.js")
    if(!ext.equals_view(std.string_view(".js"))) { env.error("/a/b/c/test.js should give .js") }
}

@test
func test_mime_extension_dot_only(env : &mut TestEnv) {
    var ext = mime::extension(".gitignore")
    if(!ext.equals_view(std.string_view(".gitignore"))) { env.error(".gitignore should give .gitignore") }
}

// ---------------------------------------------------------------------------
// is_text / is_image tests
// ---------------------------------------------------------------------------

@test
func test_mime_is_text(env : &mut TestEnv) {
    if(!mime::is_text("text/html")) { env.error("text/html is text") }
    if(!mime::is_text("application/json")) { env.error("application/json is text") }
    if(mime::is_text("image/png")) { env.error("image/png is not text") }
}

@test
func test_mime_is_image(env : &mut TestEnv) {
    if(!mime::is_image("image/png")) { env.error("image/png is image") }
    if(!mime::is_image("image/jpeg")) { env.error("image/jpeg is image") }
    if(mime::is_image("text/html")) { env.error("text/html is not image") }
}
