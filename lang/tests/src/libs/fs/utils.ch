using namespace std;

// Small helpers for fs tests. Keep things C-like and explicit.

public func env_fail(env : &mut TestEnv, msg : *char) {
    env.error(msg);
}

public func env_info(env : &mut TestEnv, msg : *char) {
    env.info(msg);
}

public func expect_true(env : &mut TestEnv, cond : bool, msg : *char) : bool {
    if(!cond) {
        env.error(msg);
        return false;
    }
    return true;
}

public func expect_eq_usize(env : &mut TestEnv, actual : size_t, expected : size_t, msg : *char) : bool {
    if(actual != expected) {
        env.error(msg);
        return false;
    }
    return true;
}

public func expect_cstr_equals(env : &mut TestEnv, a : *char, b : *char, msg : *char) : bool {
    var i : size_t = 0;
    while(true) {
        var ca = a[i];
        var cb = b[i];
        if(ca != cb) {
            env.error(msg);
            return false;
        }
        if(ca == '\0') {
            break;
        }
        i++;
    }
    return true;
}

public func make_test_base_dir(env : &mut TestEnv) : std::string {
    // Unique-ish per test id; relative path keeps it portable.
    var base = std::string("tmp_fs_tests_");
    base.append_integer(env.get_test_id());
    return base;
}

public func make_child_path(parent : &std::string, child : *char) : std::string {
    var out = std::string();
    out.append_char_ptr(parent.data());
    // always use '/' in tests; fs normalizes on windows where needed
    out.append('/');
    out.append_char_ptr(child);
    return out;
}

public func bytes_equal(buf : *u8, len : size_t, cstr : *char, msg : *char, env : &mut TestEnv) : bool {
    var want = strlen(cstr) as size_t;
    if(len != want) {
        env.error(msg);
        return false;
    }
    var i : size_t = 0;
    while(i < want) {
        if(buf[i] != (cstr[i] as u8)) {
            env.error(msg);
            return false;
        }
        i++;
    }
    return true;
}

