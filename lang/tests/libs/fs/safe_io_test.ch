using namespace std;
using namespace fs;

func test_pathbuf(p : &std::string) : path::PathBuf {
    return path::PathBuf.from(std::str.make_view(p.to_view()))
}

@test
func test_safe_write_and_read_file(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    var base_path = test_pathbuf(&base)
    fs::remove_dir_all(&base_path.as_path());
    fs::create_dir_all(&base_path.as_path());
    var p = make_child_path(&base, "safe_io.txt");
    var path = test_pathbuf(&p)

    var data : [5]u8 = ['h', 'e', 'l', 'l', 'o']
    var view : [u8] = data
    var w = fs::write(&path.as_path(), view)
    if(w is Result.Err) {
        env.error("safe write failed");
        fs::remove_dir_all(&base_path.as_path());
        return
    }

    var r = fs::read(&path.as_path())
    if(r is Result.Err) {
        env.error("safe read failed")
    } else {
        var Ok(contents) = r else unsafe { unreachable }
        expect_eq_usize(env, contents.size(), 5, "safe read size")
    }
    var text = fs::read_to_string(&path.as_path())
    if(text is Result.Err) {
        env.error("read_to_string failed")
    } else {
        var Ok(s) = text else unsafe { unreachable }
        if(!s.to_view().equals(string_view("hello"))) { env.error("read_to_string mismatch") }
    }
    fs::remove_dir_all(&base_path.as_path())
}

@test
func test_safe_file_explicit_close(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    var base_path = test_pathbuf(&base)
    fs::remove_dir_all(&base_path.as_path());
    fs::create_dir_all(&base_path.as_path());
    var p = make_child_path(&base, "safe_close.txt");
    var path = test_pathbuf(&p)

    var opened = fs::File.create(&path.as_path())
    if(opened is Result.Err) {
        env.error("safe create failed");
        fs::remove_dir_all(&base_path.as_path());
        return
    }
    var Ok(f) = opened else unsafe { unreachable }
    var data : [3]u8 = ['a', 'b', 'c']
    var view : [u8] = data
    var wr = f.write_all(view)
    if(wr is Result.Err) { env.error("safe File.write_all failed") }
    var closed = fs::close_file(&mut f)
    if(closed is Result.Err) { env.error("safe close_file failed") }
    if(f.is_open()) { env.error("closed File must report is_open() == false") }
    var read_buf : [3]u8
    var read_view = unsafe(std::mut_span<u8>(&raw mut read_buf[0], 3 as size_t))
    var rr = f.read(read_view)
    if(rr is Result.Ok) { env.error("read on a closed File must fail") }
    fs::remove_dir_all(&base_path.as_path())
}

@test
func test_safe_directory_and_metadata(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    var base_path = test_pathbuf(&base)
    fs::remove_dir_all(&base_path.as_path());
    var cr = fs::create_dir_all(&base_path.as_path())
    if(cr is Result.Err) { env.error("create_dir_all failed"); return }
    if(!fs::exists(&base_path.as_path())) { env.error("exists false after mkdir") }
    if(!fs::is_dir(&base_path.as_path())) { env.error("is_dir false") }

    var child = make_child_path(&base, "safe_dir_file.txt")
    var child_path = test_pathbuf(&child)
    var data : [3]u8 = ['a', 'b', 'c']
    var view : [u8] = data
    var wr = fs::write(&child_path.as_path(), view)
    if(wr is Result.Err) { env.error("write for metadata failed") }
    var md = fs::metadata(&child_path.as_path())
    if(md is Result.Err) {
        env.error("metadata failed")
    } else {
        var Ok(m) = md else unsafe { unreachable }
        if(!m.is_file || m.len != 3u) { env.error("metadata fields") }
    }
    if(!fs::is_file(&child_path.as_path())) { env.error("is_file false") }

    var cp = make_child_path(&base, "safe_dir_copy.txt")
    var cp_path = test_pathbuf(&cp)
    var cpr = fs::copy_file(&child_path.as_path(), &cp_path.as_path())
    if(cpr is Result.Err) { env.error("copy_file failed") }
    var mv = make_child_path(&base, "safe_dir_moved.txt")
    var mv_path = test_pathbuf(&mv)
    var mvr = fs::move_path(&cp_path.as_path(), &mv_path.as_path())
    if(mvr is Result.Err) { env.error("move_path failed") }

    var rd = fs::read_dir(&base_path.as_path())
    if(rd is Result.Err) {
        env.error("read_dir failed")
    } else {
        var Ok(entries) = rd else unsafe { unreachable }
        if(entries.size() != 2) { env.error("read_dir size") }
        var i : size_t = 0
        while(i < entries.size()) {
            unsafe {
            var e = entries.get_ptr(i)
            if(e.file_name().size() == 0) { env.error("DirEntry.file_name empty") }
            if(e.path().size() == 0) { env.error("DirEntry.path empty") }
            var em = e.metadata()
            if(em is Result.Err) {
                var Err(eme) = em else unsafe { unreachable }
                var emsg = eme.message()
                test_env::ffi::log_cstr(env, LogType.Information, emsg.data(), 0, 0)
                env.error("DirEntry.metadata failed")
            }
            }
            i += 1
        }
    }

    var rm = fs::remove_file(&mv_path.as_path())
    if(rm is Result.Err) { env.error("remove_file failed") }
    var rr = fs::remove_dir_all(&base_path.as_path())
    if(rr is Result.Err) { env.error("remove_dir_all failed") }
}

@test
func test_safe_file_seek(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    var base_path = test_pathbuf(&base)
    fs::remove_dir_all(&base_path.as_path());
    fs::create_dir_all(&base_path.as_path());
    var p = make_child_path(&base, "seek.txt")
    var path = test_pathbuf(&p)

    var data : [4]u8 = ['a', 'b', 'c', 'd']
    var view : [u8] = data
    var w = fs::write(&path.as_path(), view)
    if(w is Result.Err) { env.error("seek fixture write failed"); return }

    var opened = fs::File.open(&path.as_path(), &fs::OpenOptions.make())
    if(opened is Result.Err) { env.error("seek open failed"); return }
    var Ok(f) = opened else unsafe { unreachable }
    var sr = f.seek(std::io::SeekFrom.Start(2))
    if(sr is Result.Err) { env.error("seek failed") }
    var one : [1]u8
    var rv = unsafe(std::mut_span<u8>(&raw mut one[0], 1 as size_t))
    var r = f.read(rv)
    if(r is Result.Err) {
        env.error("read after seek failed")
    } else {
        var Ok(n) = r else unsafe { unreachable }
        if(n != 1 || one[0] != 'c') { env.error("seek position wrong") }
    }
    var m = f.metadata()
    if(m is Result.Err) { env.error("File.metadata failed") }
    fs::remove_dir_all(&base_path.as_path())
}
