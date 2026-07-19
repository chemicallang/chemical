using namespace std;

// ---------------------------------------------------------------------------
// Path Library Edge-Case Tests
// ---------------------------------------------------------------------------

@test
func test_path_basename_trailing_slash(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("/usr/bin/", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 3 && buf[0] == 'b' && buf[1] == 'i' && buf[2] == 'n' && buf[3] == '\0')) {
        env.error("basename /usr/bin/ should return bin");
    }
}

@test
func test_path_basename_double_slash(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("//usr//bin//", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 3 && buf[0] == 'b' && buf[1] == 'i' && buf[2] == 'n' && buf[3] == '\0')) {
        env.error("basename //usr//bin// should return bin");
    }
}

@test
func test_path_basename_single_char(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("/a", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == 'a' && buf[1] == '\0')) {
        env.error("basename /a should return a");
    }
}

@test
func test_path_basename_buffer_small(env : &mut TestEnv) {
    var buf : [2]char;
    var r = path::basename("toolong", &raw mut buf[0], 2);
    if(!(r is Result.Err)) {
        env.error("basename should fail with BufferTooSmall for len=2");
    }
}

@test
func test_path_dirname_no_dir(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::dirname("file.txt", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("dirname failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '.' && buf[1] == '\0')) {
        env.error("dirname file.txt should return .");
    }
}

@test
func test_path_dirname_empty(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::dirname("", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("dirname failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '.' && buf[1] == '\0')) {
        env.error("dirname of empty should return .");
    }
}

@test
func test_path_join_empty_a(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::join("", "bin", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("join failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 3 && buf[0] == 'b' && buf[1] == 'i' && buf[2] == 'n' && buf[3] == '\0')) {
        env.error("join empty + bin should return bin");
    }
}

@test
func test_path_join_empty_both(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::join("", "", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("join failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("join empty + empty should return empty");
    }
}

@test
func test_path_join_buffer_small(env : &mut TestEnv) {
    var buf : [2]char;
    var r = path::join("longer", "path", &raw mut buf[0], 2);
    if(!(r is Result.Err)) {
        env.error("join should fail with BufferTooSmall");
    }
}

@test
func test_path_normalize_deep_dotdot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("a/../../b", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 3 && buf[0] == '.' && buf[1] == '.' && buf[2] == '/' &&
         buf[3] == 'b' && buf[4] == '\0')) {
        env.error("normalize a/../../b should return ../b");
    }
}

@test
func test_path_normalize_root_dotdot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("/../usr", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' && buf[4] == '\0')) {
        env.error("normalize /../usr should return /usr");
    }
}

@test
func test_path_normalize_just_dot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize(".", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '.' && buf[1] == '\0')) {
        env.error("normalize . should return .");
    }
}

@test
func test_path_normalize_just_dotdot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("..", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 2 && buf[0] == '.' && buf[1] == '.' && buf[2] == '\0')) {
        env.error("normalize .. should return ..");
    }
}

@test
func test_path_normalize_tricky(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("foo/.../bar", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 10 && buf[0] == 'f' && buf[1] == 'o' && buf[2] == 'o' && buf[3] == '/' &&
         buf[4] == '.' && buf[5] == '.' && buf[6] == '.' && buf[7] == '/' &&
         buf[8] == 'b' && buf[9] == 'a' && buf[10] == 'r' && buf[11] == '\0')) {
        env.error("normalize foo/.../bar should keep ...");
    }
}

@test
func test_path_extension_multi_dot(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::extension("file.tar.gz", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("extension failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 3 && buf[0] == '.' && buf[1] == 'g' && buf[2] == 'z' && buf[3] == '\0')) {
        env.error("extension file.tar.gz should return .gz");
    }
}

@test
func test_path_extension_no_dot(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::extension("noext", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("extension failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("extension noext should return empty");
    }
}

@test
func test_path_extension_trailing_dot(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::extension("file.", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("extension failed"); return; }
    var Ok(len) = r else unreachable;
    // A trailing dot is part of the name in our implementation, so extension returns empty
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("extension file. should return empty");
    }
}

@test
func test_path_stem_multi_dot(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::stem("file.tar.gz", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("stem failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'e' &&
         buf[4] == '.' && buf[5] == 't' && buf[6] == 'a' && buf[7] == 'r' && buf[8] == '\0')) {
        env.error("stem file.tar.gz should return file.tar");
    }
}

@test
func test_path_is_absolute_root(env : &mut TestEnv) {
    if(!path::is_absolute("/")) {
        env.error("is_absolute / should be true");
    }
}

@test
func test_path_is_absolute_empty(env : &mut TestEnv) {
    if(path::is_absolute("")) {
        env.error("is_absolute empty should be false");
    }
}

@test
func test_path_is_absolute_just_dot(env : &mut TestEnv) {
    if(path::is_absolute(".")) {
        env.error("is_absolute . should be false");
    }
}

@test
func test_path_has_root_relative(env : &mut TestEnv) {
    if(path::has_root("usr/bin")) {
        env.error("has_root usr/bin should be false");
    }
}

@test
func test_path_has_root_empty(env : &mut TestEnv) {
    if(path::has_root("")) {
        env.error("has_root empty should be false");
    }
}

@test
func test_path_normalize_buffer_small(env : &mut TestEnv) {
    var buf : [1]char;
    var r = path::normalize("/usr", &raw mut buf[0], 1);
    if(!(r is Result.Err)) {
        env.error("normalize should fail with BufferTooSmall");
    }
}

@test
func test_path_parent_basic(env : &mut TestEnv) {
    var sv = std::string_view("/usr/bin/gcc");
    var p = path::parent(sv);
    if(!p.to_view().equals(std::string_view("/usr/bin/"))) {
        env.error("parent of /usr/bin/gcc should return /usr/bin/");
    }
}

@test
func test_path_parent_root(env : &mut TestEnv) {
    var sv = std::string_view("/");
    var p = path::parent(sv);
    if(!p.to_view().equals(std::string_view(""))) {
        env.error("parent of / should return empty");
    }
}

@test
func test_path_parent_no_sep(env : &mut TestEnv) {
    var sv = std::string_view("file.txt");
    var p = path::parent(sv);
    if(!p.to_view().equals(std::string_view(""))) {
        env.error("parent of file.txt should return empty");
    }
}
