using namespace std;
using namespace fs;

@test
func test_fs_directory_creation_and_removal(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::raw::remove_dir_all_recursive(base.data());
    
    // Create single dir
    var r1 = fs::raw::create_dir(base.data());
    expect_true(env, !(r1 is Result.Err), "create_dir failed");
    expect_true(env, fs::raw::exists(base.data()), "dir should exist after create_dir");

    // Remove single dir
    var r2 = fs::raw::remove_dir(base.data());
    expect_true(env, !(r2 is Result.Err), "remove_dir failed");
    expect_true(env, !fs::raw::exists(base.data()), "dir should not exist after remove_dir");

    // Create nested dirs
    var nested = make_child_path(&base, "a/b/c");
    var r3 = fs::raw::create_dir_all(nested.data());
    expect_true(env, !(r3 is Result.Err), "create_dir_all failed");
    expect_true(env, fs::raw::exists(nested.data()), "nested dir should exist");

    // Remove recursive
    var r4 = fs::raw::remove_dir_all_recursive(base.data());
    expect_true(env, !(r4 is Result.Err), "remove_dir_all_recursive failed");
    expect_true(env, !fs::raw::exists(base.data()), "base dir should be gone after recursive removal");
}

@test
func test_fs_directory_iteration(env : &mut TestEnv) {
        unsafe {
        var base = make_test_base_dir(env);
        fs::raw::remove_dir_all_recursive(base.data());
        fs::raw::create_dir_all(base.data());

        var f1 = make_child_path(&base, "file1.txt");
        var f2 = make_child_path(&base, "file2.txt");
        var d1 = make_child_path(&base, "subdir");
    
        fs::raw::write_text_file(f1.data(), "1" as *u8, 1);
        fs::raw::write_text_file(f2.data(), "2" as *u8, 1);
        fs::raw::create_dir(d1.data());

        var file_count = 0;
        var dir_count = 0;
    
        var r = unsafe(fs::raw::read_dir(base.data(), |&file_count, &dir_count|(name : *char, len : size_t, is_dir : bool) => {
            // skip . and .. (though read_dir implementation usually filters them, let's be safe)
            if(len == 1 && name[0] == '.') { return true; }
            if(len == 2 && name[0] == '.' && name[1] == '.') { return true; }
        
            if(is_dir) {
                dir_count++;
            } else {
                file_count++;
            }
            return true;
        }));

        expect_true(env, !(r is Result.Err), "read_dir failed");
        expect_true(env, file_count == 2, "should have 2 files");
        expect_true(env, dir_count == 1, "should have 1 directory");

        fs::raw::remove_dir_all_recursive(base.data());

        }
    }

@test
func test_fs_temp_dir(env : &mut TestEnv) {
    var buf : [PATH_MAX_BUF]char;
    var r = fs::raw::temp_dir(&raw mut buf[0], PATH_MAX_BUF as size_t);
    expect_true(env, !(r is Result.Err), "temp_dir failed");
    var Ok(len) = r else unsafe { unreachable };
    expect_true(env, len > 0, "temp_dir path should not be empty");
    expect_true(env, fs::raw::exists(&raw mut buf[0]), "temp_dir should exist");
}

@test
func test_fs_directory_rejects_null_inputs(env : &mut TestEnv) {
    var meta = fs::raw::metadata(null);
    expect_true(env, meta is Result.Err, "metadata(null) should fail closed");

    var rd = fs::raw::read_dir(null, ||(name : *char, len : size_t, is_dir : bool) => true);
    expect_true(env, rd is Result.Err, "read_dir(null) should fail closed");

    var tree = fs::raw::remove_dir_all_recursive(null);
    expect_true(env, tree is Result.Err, "remove_dir_all_recursive(null) should fail closed");
    var cd = fs::raw::create_dir(null);
    expect_true(env, cd is Result.Err, "create_dir(null) should fail closed");
    var rd2 = fs::raw::remove_dir(null);
    expect_true(env, rd2 is Result.Err, "remove_dir(null) should fail closed");
    var rf = fs::raw::remove_file(null);
    expect_true(env, rf is Result.Err, "remove_file(null) should fail closed");
}

@test
func test_fs_copy_directory_recursive(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::raw::remove_dir_all_recursive(base.data());
    
    var src = make_child_path(&base, "src");
    var dst = make_child_path(&base, "dst");
    var nested_src = make_child_path(&src, "inner");
    var file_src = make_child_path(&nested_src, "test.txt");

    fs::raw::create_dir_all(nested_src.data());
    fs::raw::write_text_file(file_src.data(), "hello" as *u8, 5);

    var r = fs::raw::copy_directory(src.data(), dst.data(), true);
    expect_true(env, !(r is Result.Err), "copy_directory failed");

    var file_dst = make_child_path(make_child_path(&dst, "inner"), "test.txt");
    expect_true(env, fs::raw::exists(file_dst.data()), "copied file should exist");

    var rd = fs::raw::read_entire_file(file_dst.data());
    expect_true(env, !(rd is Result.Err), "reading copied file failed");
    var Ok(vec) = rd else unsafe { unreachable };
    expect_true(env, vec.size() == 5, "copied file size mismatch");

    fs::raw::remove_dir_all_recursive(base.data());
}
