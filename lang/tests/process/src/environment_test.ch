using std::string;
using std::string_view;
using std::Option;
using std::Result;

@test
public func test_environment_set_get(env : &mut TestEnv) {
    var set_res = unsafe(environment::set("CHEMICAL_TEST_VAR", "test_value"))
    if(set_res is Result.Err) {
        env.error("environment::set failed")
        return
    }
    var got = environment::get("CHEMICAL_TEST_VAR")
    if(got is Option.None) {
        env.error("environment::get returned None after set")
        return
    }
    var Some(v) = got else unsafe { unreachable }
    if(!string_eq(&raw v, string_view("test_value"))) {
        env.error("environment::get returned wrong value")
        return
    }
    var unset_res = unsafe(environment::unset("CHEMICAL_TEST_VAR"))
    if(unset_res is Result.Err) {
        env.error("environment::unset failed")
        return
    }
    var after = environment::get("CHEMICAL_TEST_VAR")
    if(after is Option.Some) {
        env.error("environment::get returned Some after unset")
        return
    }
}

@test
public func test_environment_get_or(env : &mut TestEnv) {
    var v = environment::get_or("CHEMICAL_TEST_MISSING_VAR_XYZ", "default_value")
    if(!string_eq(&raw v, string_view("default_value"))) {
        env.error("environment::get_or returned wrong default")
        return
    }
}

@test
public func test_environment_path_exists(env : &mut TestEnv) {
    var p = environment::path_env()
    if(p is Option.None) {
        env.error("environment::path returned None")
        return
    }
}

@test
public func test_environment_new_surface(env : &mut TestEnv) {
    var r = environment::var("PATH")
    if(r is Result.Err) { env.error("var(PATH) should succeed"); return }
    var Ok(path_value) = r else unsafe { unreachable }
    if(path_value.size() == 0) { env.error("PATH should not be empty"); return }

    var os = environment::var_os("PATH")
    if(os is Option.None) { env.error("var_os(PATH) should be Some"); return }

    var args = environment::args()
    if(args is Result.Err) {
        env.error("args() should succeed")
    } else {
        var Ok(argv) = args else unsafe { unreachable }
        if(argv.size() == 0) { env.error("args() should include argv[0]") }
    }

    var set_res = unsafe(environment::set_var("CHEMICAL_NEW_SURFACE", "yes"))
    var got = environment::var("CHEMICAL_NEW_SURFACE")
    if(got is Result.Err) {
        env.error("var after set_var failed")
    } else {
        var Ok(gv) = got else unsafe { unreachable }
        if(!gv.to_view().equals(string_view("yes"))) { env.error("var value mismatch") }
    }
    var rm_res = unsafe(environment::remove_var("CHEMICAL_NEW_SURFACE"))
    if(environment::var("CHEMICAL_NEW_SURFACE") is Result.Ok) {
        env.error("remove_var did not remove the variable")
    }
}
