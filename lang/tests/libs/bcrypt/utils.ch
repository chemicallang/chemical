using namespace std;

public func expect_true(env : &mut TestEnv, cond : bool, msg : *char) : bool {
    if(!cond) {
        test_env::ffi::log_cstr(env, LogType.Error, msg, 0, 0);
        return false;
    }
    return true;
}

public func expect_false(env : &mut TestEnv, cond : bool, msg : *char) : bool {
    if(cond) {
        test_env::ffi::log_cstr(env, LogType.Error, msg, 0, 0);
        return false;
    }
    return true;
}

public func expect_uint_eq(env : &mut TestEnv, actual : uint, expected : uint, msg : *char) : bool {
    if(actual != expected) {
        test_env::ffi::log_cstr(env, LogType.Error, msg, 0, 0);
        return false;
    }
    return true;
}

public func expect_size_eq(env : &mut TestEnv, actual : size_t, expected : size_t, msg : *char) : bool {
    if(actual != expected) {
        test_env::ffi::log_cstr(env, LogType.Error, msg, 0, 0);
        return false;
    }
    return true;
}
