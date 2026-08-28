// threaded_test.ch — Regression test for the multi-threaded process::execute
// deadlock described in lang/docs/known-issues-process-library.md.
//
// The deadlock occurred because the old fork()-based implementation called
// getenv()/malloc() in the forked child. When fork() ran on a worker thread
// while another thread held a lock (e.g. WebKitGTK's GLib main-context lock),
// the child would deadlock. The fix uses posix_spawn(), which performs all
// setup in the parent. This test spawns a thread and runs process::execute
// there, ensuring it completes successfully without hanging.

using std::string;
using std::string_view;
using std::vector;
using std::Result;
using std::concurrent;

// Shared state written by the worker thread and read after join().
unsafe var g_thread_exec_done : bool = false
unsafe var g_thread_exec_ok : bool = false
unsafe var g_thread_exec_code : int = -1

public func process_execute_thread_entry(arg : *void) : *void {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("threaded_execute_ok"))
    var res = process::execute(cfg)
    g_thread_exec_done = true
    if(res is Result.Err) {
        return null
    }
    var Ok(r) = res else unreachable
    g_thread_exec_code = r.status.code
    g_thread_exec_ok = r.success
    return null
}

@test
public func test_process_execute_from_thread(env : &mut TestEnv) {
    g_thread_exec_done = false
    g_thread_exec_ok = false
    g_thread_exec_code = -1

    // Run process::execute on a worker thread (the exact scenario that
    // previously deadlocked under WebKitGTK). If it deadlocks, t.join()
    // would never return and the test would time out.
    var t = concurrent::spawn(process_execute_thread_entry, null)
    t.join()

    if(!g_thread_exec_done) {
        env.error("threaded process::execute did not complete (possible deadlock)")
        return
    }
    if(!g_thread_exec_ok || g_thread_exec_code != 0) {
        env.error("threaded process::execute did not succeed")
        return
    }
}

// Also verify that the spawned (async) child API works from a thread, since
// it shared the same fork()-based code path.
unsafe var g_thread_spawn_done : bool = false
unsafe var g_thread_spawn_ok : bool = false

public func process_spawn_thread_entry(arg : *void) : *void {
    var cfg = process::ProcessConfig.default()
    cfg.args.push(string("echo"))
    cfg.args.push(string("threaded_spawn_ok"))
    var res = process::spawn(cfg)
    g_thread_spawn_done = true
    if(res is Result.Err) {
        return null
    }
    var Ok(child) = res else unreachable
    var waited = process::wait(&raw mut child)
    if(waited is Result.Err) {
        return null
    }
    var Ok(r) = waited else unreachable
    g_thread_spawn_ok = r.success && r.status.code == 0
    return null
}

@test
public func test_process_spawn_from_thread(env : &mut TestEnv) {
    g_thread_spawn_done = false
    g_thread_spawn_ok = false

    var t = concurrent::spawn(process_spawn_thread_entry, null)
    t.join()

    if(!g_thread_spawn_done) {
        env.error("threaded process::spawn did not complete (possible deadlock)")
        return
    }
    if(!g_thread_spawn_ok) {
        env.error("threaded process::spawn did not succeed")
        return
    }
}
