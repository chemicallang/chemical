using std::Option;
using std::Result;
using std::string;
using std::string_view;

// Async wrappers over environment access (added alongside the sync API).

@test
public func test_environment_async_set_get_unset(env : &mut TestEnv) {
    var key = string_view("CHEM_ENV_ASYNC_TEST")

    var setr = async::block_on(environment::set_async(key, string_view("hello")))
    if(setr is Result.Err) {
        env.error("environment::set_async failed")
        return
    }

    var got = async::block_on(environment::get_async(key))
    if(got is Option.None) {
        env.error("environment::get_async returned None")
    } else {
        var Some(v) = got else unreachable
        if(v.size() != 5u) {
            env.error("environment::get_async value size mismatch")
        }
    }

    var un = async::block_on(environment::unset_async(key))
    if(un is Result.Err) {
        env.error("environment::unset_async failed")
    }
}
