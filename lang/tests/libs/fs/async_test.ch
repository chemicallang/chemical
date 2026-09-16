using namespace std;
using namespace fs;

// Async wrappers over the blocking fs API (added alongside the sync API).
// These compile the same way as any compiler-lowered `async func`, so they run
// in the `--libs` suite on both the C and LLVM backends.

@test
func test_fs_read_entire_file_async(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());
    var p = make_child_path(&base, "async_read.txt");

    var msg : *char = "async hello";
    var len = strlen(msg) as size_t;
    var wr = fs::write_text_file(p.data(), msg as *u8, len);
    if(wr is Result.Err) {
        env.error("write_text_file failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    var r = async::block_on(fs::read_entire_file_async(p.data()));
    if(r is Result.Err) {
        env.error("read_entire_file_async failed");
    } else {
        var Ok(data) = r else unreachable
        expect_eq_usize(env, data.size(), len, "async read size mismatch");
    }

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_write_text_file_async(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());
    var p = make_child_path(&base, "async_write.txt");

    var msg : *char = "written asynchronously";
    var len = strlen(msg) as size_t;
    var wr = async::block_on(fs::write_text_file_async(p.data(), msg as *u8, len));
    if(wr is Result.Err) {
        env.error("write_text_file_async failed");
        fs::remove_dir_all_recursive(base.data());
        return;
    }

    // read it back synchronously to verify
    var rr = fs::read_entire_file(p.data());
    if(rr is Result.Err) {
        env.error("sync read-back failed");
    } else {
        var Ok(data) = rr else unreachable
        expect_eq_usize(env, data.size(), len, "async write size mismatch");
    }

    fs::remove_dir_all_recursive(base.data());
}

@test
func test_fs_atomic_write_async(env : &mut TestEnv) {
    var base = make_test_base_dir(env);
    fs::remove_dir_all_recursive(base.data());
    fs::create_dir_all(base.data());
    var p = make_child_path(&base, "async_atomic.txt");

    var msg : *char = "atomic async";
    var len = strlen(msg) as size_t;
    var wr = async::block_on(fs::atomic_write_async(p.data(), msg as *u8, len));
    if(wr is Result.Err) {
        env.error("atomic_write_async failed");
    }

    fs::remove_dir_all_recursive(base.data());
}
