using namespace std;

// ---------------------------------------------------------------------------

@test
func test_path_basename_simple(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("/usr/bin/gcc", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 3 && buf[0] == 'g' && buf[1] == 'c' && buf[2] == 'c' && buf[3] == '\0')) {
        env.error("basename /usr/bin/gcc should return gcc");
    }
}

@test
func test_path_basename_root(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("/", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '/' && buf[1] == '\0')) {
        env.error("basename of / should return /");
    }
}

@test
func test_path_basename_empty(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::basename("", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("basename failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '.' && buf[1] == '\0')) {
        env.error("basename of empty should return .");
    }
}

@test
func test_path_dirname_simple(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::dirname("/usr/bin/gcc", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("dirname failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0')) {
        env.error("dirname /usr/bin/gcc should return /usr/bin");
    }
}

@test
func test_path_dirname_root(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::dirname("/", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("dirname failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '/' && buf[1] == '\0')) {
        env.error("dirname of / should return /");
    }
}

@test
func test_path_join_simple(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::join("/usr", "bin", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("join failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0')) {
        env.error("join /usr + bin should be /usr/bin");
    }
}

@test
func test_path_join_absolute_b(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::join("/usr", "/bin", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("join failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == '/' && buf[1] == 'b' && buf[2] == 'i' && buf[3] == 'n' && buf[4] == '\0')) {
        env.error("join should return b if b is absolute");
    }
}

@test
func test_path_join_trailing_sep(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::join("/usr/", "bin", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("join failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0')) {
        env.error("join /usr/ + bin should not double separator");
    }
}

@test
func test_path_normalize_dotdot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("/usr/bin/../lib/file.txt", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 17 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'l' && buf[6] == 'i' && buf[7] == 'b' &&
         buf[8] == '/' && buf[9] == 'f' && buf[10] == 'i' && buf[11] == 'l' &&
         buf[12] == 'e' && buf[13] == '.' && buf[14] == 't' && buf[15] == 'x' &&
         buf[16] == 't' && buf[17] == '\0')) {
        env.error("normalize /usr/bin/../lib/file.txt should resolve ..");
    }
}

@test
func test_path_normalize_dot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("/usr/./bin", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0')) {
        env.error("normalize /usr/./bin should remove .");
    }
}

@test
func test_path_normalize_double_sep(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("//usr///bin", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0')) {
        env.error("normalize //usr///bin should remove double /");
    }
}

@test
func test_path_normalize_relative_dotdot(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("a/b/../c", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == 'a' && buf[1] == '/' && buf[2] == 'c' && buf[3] == '\0')) {
        env.error("normalize a/b/../c should return a/c");
    }
}

@test
func test_path_normalize_trailing_slash(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("/usr/bin/", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 8 && buf[0] == '/' && buf[1] == 'u' && buf[2] == 's' && buf[3] == 'r' &&
         buf[4] == '/' && buf[5] == 'b' && buf[6] == 'i' && buf[7] == 'n' && buf[8] == '\0')) {
        env.error("normalize /usr/bin/ should remove trailing /");
    }
}

@test
func test_path_normalize_empty(env : &mut TestEnv) {
    var buf : [4096]char;
    var r = path::normalize("", &raw mut buf[0], 4096);
    if(r is Result.Err) { env.error("normalize failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 1 && buf[0] == '.' && buf[1] == '\0')) {
        env.error("normalize of empty should return .");
    }
}

@test
func test_path_is_absolute_true(env : &mut TestEnv) {
    if(!path::is_absolute("/usr")) {
        env.error("is_absolute /usr should be true");
    }
}

@test
func test_path_is_absolute_false(env : &mut TestEnv) {
    if(path::is_absolute("relative")) {
        env.error("is_absolute relative should be false");
    }
}

@test
func test_path_has_root(env : &mut TestEnv) {
    if(!path::has_root("/usr")) {
        env.error("has_root /usr should be true");
    }
}

@test
func test_path_extension(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::extension("file.txt", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("extension failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == '.' && buf[1] == 't' && buf[2] == 'x' && buf[3] == 't' && buf[4] == '\0')) {
        env.error("extension file.txt should return .txt");
    }
}

@test
func test_path_extension_no_ext(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::extension("file", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("extension failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("extension file should return empty");
    }
}

@test
func test_path_extension_hidden(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::extension(".gitignore", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("extension failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 0 && buf[0] == '\0')) {
        env.error("extension .gitignore should return empty");
    }
}

@test
func test_path_stem(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::stem("file.txt", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("stem failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 4 && buf[0] == 'f' && buf[1] == 'i' && buf[2] == 'l' && buf[3] == 'e' && buf[4] == '\0')) {
        env.error("stem file.txt should return file");
    }
}

@test
func test_path_stem_no_ext(env : &mut TestEnv) {
    var buf : [256]char;
    var r = path::stem("README", &raw mut buf[0], 256);
    if(r is Result.Err) { env.error("stem failed"); return; }
    var Ok(len) = r else unreachable;
    if(!(len == 6 && buf[0] == 'R' && buf[1] == 'E' && buf[2] == 'A' && buf[3] == 'D' && buf[4] == 'M' && buf[5] == 'E' && buf[6] == '\0')) {
        env.error("stem README should return README");
    }
}

// ---------------------------------------------------------------------------
