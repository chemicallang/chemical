using namespace std;
using namespace fs;

@test
func test_fs_path_basename(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple file
    var r1 = fs::basename("foo/bar/baz.txt", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "basename failed for baz.txt");
    expect_cstr_equals(env, &mut buf[0], "baz.txt", "basename mismatch for baz.txt");

    // Directory
    var r2 = fs::basename("/a/b/c/", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "basename failed for c/");
    expect_cstr_equals(env, &mut buf[0], "c", "basename mismatch for c/");

    // No directory
    var r3 = fs::basename("file.txt", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "basename failed for file.txt");
    expect_cstr_equals(env, &mut buf[0], "file.txt", "basename mismatch for file.txt");

    // Root
    var r4 = fs::basename("/", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "basename failed for /");
    expect_cstr_equals(env, &mut buf[0], "/", "basename mismatch for /");

    // Windows paths (if supported by the implementation)
    var r5 = fs::basename("C:\\windows\\test.exe", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r5 is Result.Err), "basename failed for windows path");
    expect_cstr_equals(env, &mut buf[0], "test.exe", "basename mismatch for windows path");
}

@test
func test_fs_path_dirname(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple file
    var r1 = fs::dirname("foo/bar/baz.txt", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "dirname failed for baz.txt");
    expect_cstr_equals(env, &mut buf[0], "foo/bar", "dirname mismatch for baz.txt");

    // Directory
    var r2 = fs::dirname("/a/b/c/", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "dirname failed for c/");
    expect_cstr_equals(env, &mut buf[0], "/a/b", "dirname mismatch for c/");

    // No directory
    var r3 = fs::dirname("file.txt", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "dirname failed for file.txt");
    expect_cstr_equals(env, &mut buf[0], ".", "dirname mismatch for file.txt");

    // Root
    var r4 = fs::dirname("/", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "dirname failed for /");
    expect_cstr_equals(env, &mut buf[0], "/", "dirname mismatch for /");
}

@test
func test_fs_path_extension(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple extension
    var r1 = fs::extension("foo.txt", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "extension failed for foo.txt");
    expect_cstr_equals(env, &mut buf[0], "txt", "extension mismatch for foo.txt");

    // Multiple dots
    var r2 = fs::extension("archive.tar.gz", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "extension failed for archive.tar.gz");
    expect_cstr_equals(env, &mut buf[0], "gz", "extension mismatch for archive.tar.gz");

    // No extension
    var r3 = fs::extension("README", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "extension failed for README");
    expect_cstr_equals(env, &mut buf[0], "", "extension mismatch for README");

    // Path with dots but no extension
    var r4 = fs::extension("dir.name/file", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "extension failed for dir.name/file");
    expect_cstr_equals(env, &mut buf[0], "", "extension mismatch for dir.name/file");
}

@test
func test_fs_path_join(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple join
    var r1 = fs::join_path("foo", "bar", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "join_path failed");
    expect_cstr_equals(env, &mut buf[0], "foo/bar", "join_path mismatch");

    // Join with existing separator
    var r2 = fs::join_path("foo/", "bar", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "join_path failed (existing sep)");
    expect_cstr_equals(env, &mut buf[0], "foo/bar", "join_path mismatch (existing sep)");

    // Join with empty first component
    var r3 = fs::join_path("", "bar", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "join_path failed (empty first)");
    expect_cstr_equals(env, &mut buf[0], "bar", "join_path mismatch (empty first)");
}

@test
func test_fs_path_normalize(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Resolve "."
    var r1 = fs::normalize_path("a/./b", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "normalize_path failed (.)");
    expect_cstr_equals(env, &mut buf[0], "a/b", "normalize_path mismatch (.)");

    // Resolve ".."
    var r2 = fs::normalize_path("a/b/../c", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "normalize_path failed (..)");
    expect_cstr_equals(env, &mut buf[0], "a/c", "normalize_path mismatch (..)");

    // Multiple ".."
    var r3 = fs::normalize_path("a/b/../../c", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "normalize_path failed (nested ..)");
    expect_cstr_equals(env, &mut buf[0], "c", "normalize_path mismatch (nested ..)");

    // Redundant separators
    var r4 = fs::normalize_path("a//b///c", &mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "normalize_path failed (multiple /)");
    expect_cstr_equals(env, &mut buf[0], "a/b/c", "normalize_path mismatch (multiple /)");
}
