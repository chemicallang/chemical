using namespace std;

@test
func test_osrand_safe_surface(env : &mut TestEnv) {
    var bytes = osrand::random_bytes(16u)
    if(bytes is Result.Err) {
        env.error("osrand::random_bytes failed")
    } else {
        var Ok(b) = bytes else unsafe { unreachable }
        if(b.size() != 16u) { env.error("osrand::random_bytes size") }
    }
    var u = osrand::random_u32()
    if(u is Result.Err) { env.error("osrand::random_u32 failed") }
    var empty : [1]u8 = [0u8]
    var view : &mut [u8] = &mut empty
    var f = osrand::fill(view)
    if(f is Result.Err) { env.error("osrand::fill failed") }
}
