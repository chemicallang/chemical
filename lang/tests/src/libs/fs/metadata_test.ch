using namespace std;
using namespace fs;

@test
func test_fs_metadata_exists_and_types(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var file_path = make_child_path(base, "file.txt");
    var dir_path = make_child_path(base, "subdir");
    
    // Test directory
    fs::create_dir(dir_path.data());
    expect_true(env, fs::exists(dir_path.data()), "dir should exist");
    var r_dir = fs::is_dir(dir_path.data());
    expect_true(env, !(r_dir is Result.Err), "is_dir failed");
    var Ok(is_d) = r_dir else unreachable;
    expect_true(env, is_d, "should be a directory");
    
    var r_not_f = fs::is_file(dir_path.data());
    var Ok(is_f1) = r_not_f else unreachable;
    expect_true(env, !is_f1, "dir should not be a file");

    // Test file
    var content : *char = "metadata test";
    fs::write_text_file(file_path.data(), content as *u8, strlen(content) as size_t);
    expect_true(env, fs::exists(file_path.data()), "file should exist");
    
    var r_file = fs::is_file(file_path.data());
    expect_true(env, !(r_file is Result.Err), "is_file failed");
    var Ok(is_f2) = r_file else unreachable;
    expect_true(env, is_f2, "should be a file");
    
    var r_not_d = fs::is_dir(file_path.data());
    var Ok(is_d2) = r_not_d else unreachable;
    expect_true(env, !is_d2, "file should not be a dir");

    // Non-existent
    var missing = make_child_path(base, "missing");
    expect_true(env, !fs::exists(missing.data()), "missing should not exist");

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_metadata_properties(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var p = make_child_path(base, "props.txt");
    var content : *char = "1234567890";
    var len = strlen(content) as size_t;
    fs::write_text_file(p.data(), content as *u8, len);

    var r = fs::metadata(p.data());
    expect_true(env, !(r is Result.Err), "metadata failed");
    var Ok(m) = r else unreachable;
    
    expect_true(env, m.is_file, "metadata: should be file");
    expect_true(env, !m.is_dir, "metadata: should not be dir");
    expect_eq_usize(env, m.len, len, "metadata: length mismatch");
    expect_true(env, m.modified > 0, "metadata: modified time should be > 0");

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_set_permissions_and_times(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var p = make_child_path(base, "perms_times.txt");
    fs::write_text_file(p.data(), "test" as *u8, 4);

    // Set permissions
    var rp = fs::set_permissions(p.data(), 0o444 as u32); // READ_ONLY ish
    expect_true(env, !(rp is Result.Err), "set_permissions failed");

    // Set times
    var atime : i64 = 123456789;
    var mtime : i64 = 987654321;
    var rt = fs::set_times(p.data(), atime, mtime);
    expect_true(env, !(rt is Result.Err), "set_times failed");

    // Verify times
    var rm = fs::metadata(p.data());
    expect_true(env, !(rm is Result.Err), "metadata after set_times failed");
    var Ok(m) = rm else unreachable;
    // Note: Some filesystems might have precision issues, but exact match for seconds is usually fine.
    // However, on Windows, precision can be an issue. We just check they are updated.
    expect_true(env, m.modified == mtime, "modified time mismatch");
    expect_true(env, m.accessed == atime, "accessed time mismatch");

    fs::remove_dir_all_recursive(base.data());
}
