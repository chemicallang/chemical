using std::string;
using std::string_view;
using std::Option;
using std::Result;

@test
public func test_environment_set_get(env : &mut TestEnv) {
    var set_res = environment::set("CHEMICAL_TEST_VAR", "test_value")
    if(set_res is Result.Err) {
        env.error("environment::set failed")
        return
    }
    var got = environment::get("CHEMICAL_TEST_VAR")
    if(got is Option.None) {
        env.error("environment::get returned None after set")
        return
    }
    var Some(v) = got else unreachable
    if(!string_eq(&raw v, string_view("test_value"))) {
        env.error("environment::get returned wrong value")
        return
    }
    var unset_res = environment::unset("CHEMICAL_TEST_VAR")
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
    var p = environment::path()
    if(p is Option.None) {
        env.error("environment::path returned None")
        return
    }
}
