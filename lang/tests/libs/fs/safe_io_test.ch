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

@test
func test_safe_directory_and_metadata(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    var base_path = std::str.make_view(base.to_view())
    var cr = fs::create_dir_all_safe(base_path)
    if(cr is Result.Err) { env.error("create_dir_all_safe failed"); return }
    if(!fs::exists_safe(base_path)) { env.error("exists_safe false after mkdir") }
    if(!fs::is_dir_safe(base_path)) { env.error("is_dir_safe false") }

    var child = make_child_path(&base, "safe_dir_file.txt")
    var child_path = std::str.make_view(child.to_view())
    var data : [3]u8 = ['a', 'b', 'c']
    var view : [u8] = data
    var wr = fs::write_file(child_path, view)
    if(wr is Result.Err) { env.error("write_file for metadata failed") }
    var md = fs::metadata_safe(child_path)
    if(md is Result.Err) {
        env.error("metadata_safe failed")
    } else {
        var Ok(m) = md else unsafe { unreachable }
        if(!m.is_file || m.len != 3u) { env.error("metadata_safe fields") }
    }
    if(!fs::is_file_safe(child_path)) { env.error("is_file_safe false") }

    var cp = make_child_path(&base, "safe_dir_copy.txt")
    var cp_path = std::str.make_view(cp.to_view())
    var cpr = fs::copy_file_safe(child_path, cp_path)
    if(cpr is Result.Err) { env.error("copy_file_safe failed") }
    var mv = make_child_path(&base, "safe_dir_moved.txt")
    var mv_path = std::str.make_view(mv.to_view())
    var mvr = fs::move_path_safe(cp_path, mv_path)
    if(mvr is Result.Err) { env.error("move_path_safe failed") }

    var td = fs::temp_dir_path()
    if(td is Option.None) { env.error("temp_dir_path returned None") }

    var rm = fs::remove_file_safe(mv_path)
    if(rm is Result.Err) { env.error("remove_file_safe failed") }
    var rd = fs::remove_dir_all_safe(base_path)
    if(rd is Result.Err) { env.error("remove_dir_all_safe failed") }
}
