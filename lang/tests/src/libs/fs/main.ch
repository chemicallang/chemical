using namespace std;
using namespace fs;

// Mirror the libs/json testing style: @test + TestEnv.
// These tests stick to public fs APIs only.

@test
func test_fs_create_dir_all_and_remove_recursive(env : &mut TestEnv) {
    var base = make_test_base_dir(env);

    // best-effort cleanup (ignore errors)
    fs::remove_dir_all_recursive(base.data());

    var nested1 = make_child_path(base, "a");
    var nested2 = make_child_path(nested1, "b");
    var nested3 = make_child_path(nested2, "c");

    var r = fs::create_dir_all(nested3.data());
    if(r is Result.Err) {
        var Err(e) = r else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("create_dir_all failed");
        return;
    }

    // remove the whole tree
    var rem = fs::remove_dir_all_recursive(base.data());
    if(rem is Result.Err) {
        var Err(e) = rem else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("remove_dir_all_recursive failed");
        return;
    }
}

@test
func test_fs_write_and_read_to_buffer_roundtrip(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());

    var rdir = fs::create_dir_all(base.data());
    if(rdir is Result.Err) {
        var Err(e) = rdir else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("create_dir_all failed");
        return;
    }

    var file_path = make_child_path(base, "hello.txt");
    var content : *char = "hello fs module";
    var len = strlen(content) as size_t;

    var wr = fs::write_text_file(file_path.data(), content as *u8, len);
    if(wr is Result.Err) {
        var Err(e) = wr else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("write_text_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var buf : [256]u8;
    var rd = fs::read_to_buffer(file_path.data(), &mut buf[0], sizeof(buf));
    if(rd is Result.Err) {
        var Err(e) = rd else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("read_to_buffer failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var Ok(n) = rd else unreachable;
    bytes_equal(&mut buf[0], n, content, "read_to_buffer bytes mismatch", env);

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_write_and_read_entire_file_roundtrip(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());

    var rdir = fs::create_dir_all(base.data());
    if(rdir is Result.Err) {
        var Err(e) = rdir else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("create_dir_all failed");
        return;
    }

    var file_path = make_child_path(base, "bytes.bin");
    var content : *char = "0123456789abcdef";
    var len = strlen(content) as size_t;

    var wr = fs::write_text_file(file_path.data(), content as *u8, len);
    if(wr is Result.Err) {
        var Err(e) = wr else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("write_text_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var rd = fs::read_entire_file(file_path.data());
    if(rd is Result.Err) {
        var Err(e) = rd else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("read_entire_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var Ok(vec) = rd else unreachable;
    if(!bytes_equal(vec.data(), vec.size(), content, "read_entire_file bytes mismatch", env)) {
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_copy_file_copies_contents(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var src = make_child_path(base, "src.txt");
    var dst = make_child_path(base, "dst.txt");

    var content : *char = "copy-me";
    var len = strlen(content) as size_t;
    var wr = fs::write_text_file(src.data(), content as *u8, len);
    if(wr is Result.Err) {
        var Err(e) = wr else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("write_text_file(src) failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var cp = fs::copy_file(src.data(), dst.data());
    if(cp is Result.Err) {
        var Err(e) = cp else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("copy_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var rd = fs::read_entire_file(dst.data());
    if(rd is Result.Err) {
        var Err(e) = rd else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("read_entire_file(dst) failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }
    var Ok(vec) = rd else unreachable;
    bytes_equal(vec.data(), vec.size(), content, "copy_file contents mismatch", env);

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_remove_file_makes_file_unreadable(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var p = make_child_path(base, "toremove.txt");
    var content : *char = "bye";
    var wr = fs::write_text_file(p.data(), content as *u8, strlen(content) as size_t);
    if(wr is Result.Err) {
        var Err(e) = wr else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("write_text_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var rm = fs::remove_file(p.data());
    if(rm is Result.Err) {
        var Err(e) = rm else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("remove_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    // Should fail to read after deletion
    var rd = fs::read_entire_file(p.data());
    if(!(rd is Result.Err)) {
        env.error("expected read_entire_file to fail after remove_file");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_remove_dir_recursive_removes_nested_files(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());

    var d1 = make_child_path(base, "dir1");
    var d2 = make_child_path(d1, "dir2");
    var d3 = make_child_path(d2, "dir3");
    var mk = fs::create_dir_all(d3.data());
    if(mk is Result.Err) {
        var Err(e) = mk else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("create_dir_all failed");
        return;
    }

    // Put files at multiple depths
    var f1 = make_child_path(base, "root.txt");
    var f2 = make_child_path(d2, "mid.txt");
    var f3 = make_child_path(d3, "deep.txt");

    var c1 : *char = "root";
    var c2 : *char = "mid";
    var c3 : *char = "deep";
    fs::write_text_file(f1.data(), c1 as *u8, strlen(c1) as size_t);
    fs::write_text_file(f2.data(), c2 as *u8, strlen(c2) as size_t);
    fs::write_text_file(f3.data(), c3 as *u8, strlen(c3) as size_t);

    var rm = fs::remove_dir_all_recursive(base.data());
    if(rm is Result.Err) {
        var Err(e) = rm else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("remove_dir_all_recursive failed");
        return;
    }

    // directory should be gone -> recreate should succeed
    var mk2 = fs::create_dir_all(base.data());
    if(mk2 is Result.Err) {
        var Err(e) = mk2 else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("expected create_dir_all to work after removing tree");
        return;
    }

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_set_permissions_on_existing_file(env : &mut TestEnv) {
    // Don't assert specific permission bits (platform differences); just ensure API works.
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());

    var p = make_child_path(base, "perm.txt");
    var content : *char = "perm";
    var wr = fs::write_text_file(p.data(), content as *u8, strlen(content) as size_t);
    if(wr is Result.Err) {
        var Err(e) = wr else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("write_text_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    // POSIX-like perms (rw-r--r--). Windows mapping is coarse but should not crash.
    var sp = fs::set_permissions(p.data(), 0o644 as u32);
    if(sp is Result.Err) {
        var Err(e) = sp else unreachable;
        var m = e.message();
        env.error(m.data());
        env.error("set_permissions failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_errors_for_missing_paths(env : &mut TestEnv) {
    // Use a path that should not exist.
    var base = make_test_base_dir(env);
    var missing = make_child_path(base, "definitely_missing_file.txt");

    var rd = fs::read_entire_file(missing.data());
    if(!(rd is Result.Err)) {
        env.error("expected read_entire_file(missing) to fail");
        return;
    }

    var rm = fs::remove_file(missing.data());
    if(!(rm is Result.Err)) {
        env.error("expected remove_file(missing) to fail");
        return;
    }
}

