using namespace std;
using namespace fs;

@test
func test_fs_file_low_level_api(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::raw::remove_dir_all_recursive(base.data());
    fs::raw::create_dir_all(base.data());

    var p = make_child_path(&base, "low_level.bin");
    
    // Create and write
    var opts : OpenOptions;
    opts.read = true;
    opts.write = true;
    opts.create = true;
    opts.truncate = true;
    opts.binary = true;

    var r_open = fs::raw::file_open(p.data(), opts);
    expect_true(env, !(r_open is Result.Err), "file_open (create) failed");
    var Ok(f) = r_open else unsafe { unreachable };
    
    var data : *char = "chemical low level";
    var len = unsafe(strlen(data)) as size_t;
    var r_write = fs::raw::file_write(&raw mut f, data as *u8, len);
    expect_true(env, !(r_write is Result.Err), "file_write failed");
    var Ok(n_written) = r_write else unsafe { unreachable };
    expect_eq_usize(env, n_written, len, "written size mismatch");

    fs::raw::file_close(&raw mut f);

    // Open and read
    opts.create = false;
    opts.truncate = false;
    var r_open2 = fs::raw::file_open(p.data(), opts);
    expect_true(env, !(r_open2 is Result.Err), "file_open (read) failed");
    var Ok(f2) = r_open2 else unsafe { unreachable };

    var buf : [64]u8;
    var r_read = fs::raw::file_read(&raw mut f2, &raw mut buf[0], 64);
    expect_true(env, !(r_read is Result.Err), "file_read failed");
    var Ok(n_read) = r_read else unsafe { unreachable };
    expect_eq_usize(env, n_read, len, "read size mismatch");
    expect_true(env, bytes_equal(&raw mut buf[0], n_read, data, "content mismatch", env), "content mismatch");

    fs::raw::file_close(&raw mut f2);
    fs::raw::remove_dir_all_recursive(base.data());
}

@test
func test_fs_atomic_write(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::raw::remove_dir_all_recursive(base.data());
    fs::raw::create_dir_all(base.data());

    var p = make_child_path(&base, "atomic.txt");
    var data : *char = "atomic content";
    var len = unsafe(strlen(data)) as size_t;

    var r = fs::raw::atomic_write(p.data(), data as *u8, len);
    expect_true(env, !(r is Result.Err), "atomic_write failed");
    expect_true(env, fs::raw::exists(p.data()), "atomic file should exist");

    var rd = fs::raw::read_entire_file(p.data());
    expect_true(env, !(rd is Result.Err), "read_entire_file failed");
    var Ok(vec) = rd else unsafe { unreachable };
    expect_true(env, vec.size() == len, "size mismatch");

    fs::raw::remove_dir_all_recursive(base.data());
}

@test
func test_fs_move_path(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::raw::remove_dir_all_recursive(base.data());
    fs::raw::create_dir_all(base.data());

    var src = make_child_path(&base, "source.txt");
    var dst = make_child_path(&base, "dest.txt");
    var data : *char = "move me";
    
    fs::raw::write_text_file(src.data(), data as *u8, unsafe(strlen(data)) as size_t);

    var r = fs::raw::move_path(src.data(), dst.data());
    expect_true(env, !(r is Result.Err), "move_path failed");
    expect_true(env, !fs::raw::exists(src.data()), "source should not exist after move");
    expect_true(env, fs::raw::exists(dst.data()), "destination should exist after move");

    var rd = fs::raw::read_entire_file(dst.data());
    var Ok(vec) = rd else unsafe { unreachable };
    expect_true(env, vec.size() == unsafe(strlen(data)) as size_t, "content size mismatch");

    fs::raw::remove_dir_all_recursive(base.data());
}
