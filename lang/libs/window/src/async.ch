// ---------------------------------------------------------------------------
// UI-thread async bridge (Tier 5).
//
// The async runtime's executor is a cooperative poll loop. A GUI app spends its
// time inside the native event loop (`gtk_main` / `GetMessage`), so a future
// submitted with `async::spawn_local` would never be polled on its own. This
// file couples the executor to the native loop:
//
//   * `window_run_async` installs a wake hook and a periodic tick, then runs the
//     native loop; the executor is polled on the UI thread.
//   * the wake hook posts to the native loop (`g_idle_add` / `WM_NULL`), so a
//     completion on a worker thread (e.g. `async::spawn_blocking`) re-polls
//     promptly instead of waiting for the next tick.
//   * `window_pump` exposes one iteration for custom loops and tests.
//
// Because every task is polled on the UI thread, the continuation after an
// `await` also runs on the UI thread: an async handler may touch windows/widgets
// directly after awaiting background work. Blocking work must still be wrapped
// in `async::spawn_blocking` (the universal escape hatch) so it runs off the UI
// thread while the loop stays responsive.
//
// Platform backends (`posix/linux.ch`, `win/win.ch`) implement the three
// `window_async_platform_*` hooks; this file holds the cross-platform logic.
// ---------------------------------------------------------------------------

public namespace window {

// Poll the async executor and drain pending native events once. Call this from
// a custom event loop, or repeatedly from a test, to drive UI-spawned tasks.
public func window_pump() {
    async::executor_poll_tasks(async::executor(), 4096u)
    window_async_platform_pump()
    async::executor_poll_tasks(async::executor(), 4096u)
}

// Like `window_run`, but drives the async executor from the event loop. Use
// this instead of `window_run` when the app uses `async::spawn_local`.
public func window_run_async() {
    var e = async::executor()
    async::executor_set_wake_hook(e, window_async_platform_wake, null)
    window_async_platform_install_pump()
    window_run()
    async::executor_set_wake_hook(e, null, null)
}

} // end namespace window
