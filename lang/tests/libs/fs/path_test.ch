using namespace std;
using namespace fs;

@test
func test_fs_path_basename(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple file
    var r1 = fs::raw::basename("foo/bar/baz.txt", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "basename failed for baz.txt");
    expect_cstr_equals(env, &raw mut buf[0], "baz.txt", "basename mismatch for baz.txt");

    // Directory
    var r2 = fs::raw::basename("/a/b/c/", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "basename failed for c/");
    expect_cstr_equals(env, &raw mut buf[0], "c", "basename mismatch for c/");

    // No directory
    var r3 = fs::raw::basename("file.txt", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "basename failed for file.txt");
    expect_cstr_equals(env, &raw mut buf[0], "file.txt", "basename mismatch for file.txt");

    // Root
    var r4 = fs::raw::basename("/", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "basename failed for /");
    expect_cstr_equals(env, &raw mut buf[0], "/", "basename mismatch for /");

    // Windows paths (if supported by the implementation)
    var r5 = fs::raw::basename("C:\\windows\\test.exe", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r5 is Result.Err), "basename failed for windows path");
    expect_cstr_equals(env, &raw mut buf[0], "test.exe", "basename mismatch for windows path");
}

@test
func test_fs_path_dirname(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple file
    var r1 = fs::raw::dirname("foo/bar/baz.txt", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "dirname failed for baz.txt");
    expect_cstr_equals(env, &raw mut buf[0], "foo/bar", "dirname mismatch for baz.txt");

    // Directory
    var r2 = fs::raw::dirname("/a/b/c/", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "dirname failed for c/");
    expect_cstr_equals(env, &raw mut buf[0], "/a/b", "dirname mismatch for c/");

    // No directory
    var r3 = fs::raw::dirname("file.txt", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "dirname failed for file.txt");
    expect_cstr_equals(env, &raw mut buf[0], ".", "dirname mismatch for file.txt");

    // Root
    var r4 = fs::raw::dirname("/", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "dirname failed for /");
    expect_cstr_equals(env, &raw mut buf[0], "/", "dirname mismatch for /");
}

@test
func test_fs_path_extension(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple extension
    var r1 = fs::raw::extension("foo.txt", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "extension failed for foo.txt");
    expect_cstr_equals(env, &raw mut buf[0], "txt", "extension mismatch for foo.txt");

    // Multiple dots
    var r2 = fs::raw::extension("archive.tar.gz", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "extension failed for archive.tar.gz");
    expect_cstr_equals(env, &raw mut buf[0], "gz", "extension mismatch for archive.tar.gz");

    // No extension
    var r3 = fs::raw::extension("README", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "extension failed for README");
    expect_cstr_equals(env, &raw mut buf[0], "", "extension mismatch for README");

    // Path with dots but no extension
    var r4 = fs::raw::extension("dir.name/file", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "extension failed for dir.name/file");
    expect_cstr_equals(env, &raw mut buf[0], "", "extension mismatch for dir.name/file");
}

@test
func test_fs_path_join(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Simple join
    var r1 = fs::raw::join_path("foo", "bar", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "join_path failed");
    expect_cstr_equals(env, &raw mut buf[0], "foo/bar", "join_path mismatch");

    // Join with existing separator
    var r2 = fs::raw::join_path("foo/", "bar", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "join_path failed (existing sep)");
    expect_cstr_equals(env, &raw mut buf[0], "foo/bar", "join_path mismatch (existing sep)");

    // Join with empty first component
    var r3 = fs::raw::join_path("", "bar", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "join_path failed (empty first)");
    expect_cstr_equals(env, &raw mut buf[0], "bar", "join_path mismatch (empty first)");
}

@test
func test_fs_atomic_write_rejects_overflowing_temp_path(env : &mut TestEnv) {
    var path_buf : [PATH_MAX_BUF]char;
    var i : size_t = 0;
    while(i < 4088) { path_buf[i] = 'a'; i++ }
    // Keep a directory separator before the final component so dirname returns
    // a near-PATH_MAX parent; the temp-file suffix must then be rejected.
    path_buf[4080] = '/';
    path_buf[4088] = '/';
    path_buf[4089] = 'x';
    path_buf[4090] = 0;
    var data : *u8 = "x" as *u8;
    var r = fs::raw::atomic_write(&raw mut path_buf[0], data, 1);
    expect_true(env, r is Result.Err, "atomic_write must reject an overflowing temp path");
}

@test
func test_fs_path_null_inputs_rejected(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    var r1 = fs::raw::basename(null, &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, r1 is Result.Err, "basename must reject a null path");
    var r2 = fs::raw::join_path("a", null, &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, r2 is Result.Err, "join_path must reject a null component");
    var r3 = fs::raw::read_to_buffer("x", null, 1 as size_t);
    expect_true(env, r3 is Result.Err, "read_to_buffer must reject a null buffer");
}

@test
func test_fs_temp_dir_rejects_small_or_null_output(env : &mut TestEnv) {
    var small : [2]char;
    var r1 = fs::raw::temp_dir(&raw mut small[0], 2 as size_t);
    expect_true(env, r1 is Result.Err, "temp_dir must reject a too-small output buffer");
    var r2 = fs::raw::temp_dir(null, 0 as size_t);
    expect_true(env, r2 is Result.Err, "temp_dir must reject a null output buffer");
}

@test
func test_fs_path_normalize(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    
    // Resolve "."
    var r1 = fs::raw::normalize_path("a/./b", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r1 is Result.Err), "normalize_path failed (.)");
    expect_cstr_equals(env, &raw mut buf[0], "a/b", "normalize_path mismatch (.)");

    // Resolve ".."
    var r2 = fs::raw::normalize_path("a/b/../c", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r2 is Result.Err), "normalize_path failed (..)");
    expect_cstr_equals(env, &raw mut buf[0], "a/c", "normalize_path mismatch (..)");

    // Multiple ".."
    var r3 = fs::raw::normalize_path("a/b/../../c", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r3 is Result.Err), "normalize_path failed (nested ..)");
    expect_cstr_equals(env, &raw mut buf[0], "c", "normalize_path mismatch (nested ..)");

    // Redundant separators
    var r4 = fs::raw::normalize_path("a//b///c", &raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r4 is Result.Err), "normalize_path failed (multiple /)");
    expect_cstr_equals(env, &raw mut buf[0], "a/b/c", "normalize_path mismatch (multiple /)");
}
