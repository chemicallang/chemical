using namespace std;
using namespace fs;

@test
func test_safe_write_and_read_file(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());
    var p = make_child_path(&base, "safe_io.txt");
    var path = std::str.make_view(p.to_view())

    var data : [5]u8 = ['h', 'e', 'l', 'l', 'o']
    var view : [u8] = data
    var w = fs::write_file(path, view)
    if(w is Result.Err) {
        env.error("safe write_file failed");
        fs::remove_dir_all_recursive(base.data());
        return
    }

    var r = fs::read_file(path)
    if(r is Result.Err) {
        env.error("safe read_file failed")
    } else {
        var Ok(contents) = r else unsafe { unreachable }
        expect_eq_usize(env, contents.size(), 5, "safe read size")
    }
    fs::remove_dir_all_recursive(base.data())
}

@test
func test_safe_file_explicit_close(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());
    var p = make_child_path(&base, "safe_close.txt");
    var path = std::str.make_view(p.to_view())

    var opened = fs::create_file(path)
    if(opened is Result.Err) {
        env.error("safe create_file failed");
        fs::remove_dir_all_recursive(base.data());
        return
    }
    var Ok(f) = opened else unsafe { unreachable }
    var data : [3]u8 = ['a', 'b', 'c']
    var view : [u8] = data
    var wr = f.write_all(view)
    if(wr is Result.Err) { env.error("safe File.write_all failed") }
    var closed = fs::close_file(&mut f)
    if(closed is Result.Err) { env.error("safe close_file failed") }
    fs::remove_dir_all_recursive(base.data())
}
