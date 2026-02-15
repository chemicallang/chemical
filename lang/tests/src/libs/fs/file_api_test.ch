using namespace std;
using namespace fs;

@test
func test_fs_file_low_level_api(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var p = make_child_path(base, "low_level.bin");
    
    // Create and write
    var opts : OpenOptions;
    opts.read = true;
    opts.write = true;
    opts.create = true;
    opts.truncate = true;
    opts.binary = true;

    var r_open = fs::file_open(p.data(), opts);
    expect_true(env, !(r_open is Result.Err), "file_open (create) failed");
    var Ok(f) = r_open else unreachable;
    
    var data : *char = "chemical low level";
    var len = strlen(data) as size_t;
    var r_write = fs::file_write(&mut f, data as *u8, len);
    expect_true(env, !(r_write is Result.Err), "file_write failed");
    var Ok(n_written) = r_write else unreachable;
    expect_eq_usize(env, n_written, len, "written size mismatch");

    fs::file_close(&mut f);

    // Open and read
    opts.create = false;
    opts.truncate = false;
    var r_open2 = fs::file_open(p.data(), opts);
    expect_true(env, !(r_open2 is Result.Err), "file_open (read) failed");
    var Ok(f2) = r_open2 else unreachable;

    var buf : [64]u8;
    var r_read = fs::file_read(&mut f2, &mut buf[0], 64);
    expect_true(env, !(r_read is Result.Err), "file_read failed");
    var Ok(n_read) = r_read else unreachable;
    expect_eq_usize(env, n_read, len, "read size mismatch");
    expect_true(env, bytes_equal(&mut buf[0], n_read, data, "content mismatch", env), "content mismatch");

    fs::file_close(&mut f2);
    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_atomic_write(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var p = make_child_path(base, "atomic.txt");
    var data : *char = "atomic content";
    var len = strlen(data) as size_t;

    var r = fs::atomic_write(p.data(), data as *u8, len);
    expect_true(env, !(r is Result.Err), "atomic_write failed");
    expect_true(env, fs::exists(p.data()), "atomic file should exist");

    var rd = fs::read_entire_file(p.data());
    expect_true(env, !(rd is Result.Err), "read_entire_file failed");
    var Ok(vec) = rd else unreachable;
    expect_true(env, vec.size() == len, "size mismatch");

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_move_path(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var src = make_child_path(base, "source.txt");
    var dst = make_child_path(base, "dest.txt");
    var data : *char = "move me";
    
    fs::write_text_file(src.data(), data as *u8, strlen(data) as size_t);

    var r = fs::move_path(src.data(), dst.data());
    expect_true(env, !(r is Result.Err), "move_path failed");
    expect_true(env, !fs::exists(src.data()), "source should not exist after move");
    expect_true(env, fs::exists(dst.data()), "destination should exist after move");

    var rd = fs::read_entire_file(dst.data());
    var Ok(vec) = rd else unreachable;
    expect_true(env, vec.size() == strlen(data) as size_t, "content size mismatch");

    fs::remove_dir_all_recursive(base.data());
}
