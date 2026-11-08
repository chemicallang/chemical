// Safe MIME API: &str wrappers over get_type/extension/is_text/is_image.
//
// The legacy *_cstr entry points are kept and are checked here too.

using std::string;
using std::string_view;

@test
func test_mime_safe_get_type_empty_and_unknown(env : &mut TestEnv) {
    var empty = mime::get_type("")
    if(!empty.equals(string_view("application/octet-stream"))) {
        env.error("empty extension should be application/octet-stream")
    }
    var unknown = mime::get_type(".no-such-extension")
    if(!unknown.equals(string_view("application/octet-stream"))) {
        env.error("unknown extension should be application/octet-stream")
    }
    var unicode = mime::get_type(".文件")
    if(!unicode.equals(string_view("application/octet-stream"))) {
        env.error("unicode extension should be unknown")
    }
}

@test
func test_mime_safe_get_type_accepts_views_and_strings(env : &mut TestEnv) {
    var view = string_view(".css")
    var from_view = mime::get_type(view)
    if(!from_view.equals(string_view("text/css"))) {
        env.error("get_type should accept a string_view")
    }

    var owned = string(".json")
    var from_string = mime::get_type(std::str.make_string(&owned))
    if(!from_string.equals(string_view("application/json"))) {
        env.error("get_type should accept an owned string")
    }
}

@test
func test_mime_safe_extension_empty_and_unicode(env : &mut TestEnv) {
    var none = mime::extension("")
    if(none.size() != 0u) {
        env.error("empty path should have no extension")
    }
    var no_dot = mime::extension("/a/b/c")
    if(no_dot.size() != 0u) {
        env.error("a path without a dot should have no extension")
    }
    var unicode = mime::extension("/a/文件.txt")
    if(!unicode.equals_view(string_view(".txt"))) {
        env.error("extension should preserve the ASCII suffix")
    }
    var unicode_ext = mime::extension("/a/文件")
    if(unicode_ext.size() != 0u) {
        env.error("a unicode basename with no dot should have no extension")
    }
    var backslash = mime::extension("C:\\dir\\file.css")
    if(!backslash.equals_view(string_view(".css"))) {
        env.error("Windows separators should be handled")
    }
    var hidden = mime::extension(".gitignore")
    if(!hidden.equals_view(string_view(".gitignore"))) {
        env.error("a dotfile should be treated as an extension")
    }
}

@test
func test_mime_safe_is_text_and_is_image_empty(env : &mut TestEnv) {
    if(mime::is_text("")) { env.error("empty type must not be text") }
    if(mime::is_image("")) { env.error("empty type must not be image") }
    if(mime::is_text("text/html; charset=utf-8")) {
        env.error("parameters are not part of the exact type match")
    }
    if(!mime::is_text("text/csv")) { env.error("text/csv should be text") }
    if(!mime::is_image("image/svg+xml")) { env.error("image/svg+xml should be image") }
}

@test
func test_mime_legacy_cstr_entry_points_still_work(env : &mut TestEnv) {
    if(!mime::raw::get_type_cstr(".html").equals(string_view("text/html"))) {
        env.error("legacy get_type_cstr should still work")
    }
    if(!mime::raw::extension_cstr("/a/b.css").equals_view(string_view(".css"))) {
        env.error("legacy extension_cstr should still work")
    }
    if(!mime::raw::is_text_cstr("application/json")) {
        env.error("legacy is_text_cstr should still work")
    }
    if(!mime::raw::is_image_cstr("image/png")) {
        env.error("legacy is_image_cstr should still work")
    }
}
