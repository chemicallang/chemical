// M6.2 data-race smoke test. Run under ThreadSanitizer with
// scripts/thread-tsan.sh; the plain binary is also a normal runtime test.

func worker_increment(shared : std::Arc<std::Mutex<int> >) : int {
    var i = 0
    while(i < 1000) {
        var guard = shared.get().lock()
        var value = guard.get()
        *value = *value + 1
        i = i + 1
    }
    return 0
}

func worker_rwlock_increment(shared : std::Arc<std::RwLock<int> >) : int {
    var i = 0
    while(i < 1000) {
        var guard = shared.get().write()
        var value = guard.get()
        *value = *value + 1
        i = i + 1
    }
    return 0
}

public func main() : int {
    var shared = std::arc(std::new_mutex(0))
    var first = std::thread.spawn<std::Arc<std::Mutex<int> >, int>(
        worker_increment, shared.clone())
    var second = std::thread.spawn<std::Arc<std::Mutex<int> >, int>(
        worker_increment, shared.clone())
    var third = std::thread.spawn<std::Arc<std::Mutex<int> >, int>(
        worker_increment, shared.clone())
    var fourth = std::thread.spawn<std::Arc<std::Mutex<int> >, int>(
        worker_increment, shared.clone())
    first.join()
    second.join()
    third.join()
    fourth.join()
    var guard = shared.get().lock()
    if(*guard.get() != 4000) {
        return 1
    }

    // RwLock write guards must serialize writers across threads as well.
    var rw = std::arc(std::new_rwlock(0))
    var rw_first = std::thread.spawn<std::Arc<std::RwLock<int> >, int>(
        worker_rwlock_increment, rw.clone())
    var rw_second = std::thread.spawn<std::Arc<std::RwLock<int> >, int>(
        worker_rwlock_increment, rw.clone())
    var rw_third = std::thread.spawn<std::Arc<std::RwLock<int> >, int>(
        worker_rwlock_increment, rw.clone())
    var rw_fourth = std::thread.spawn<std::Arc<std::RwLock<int> >, int>(
        worker_rwlock_increment, rw.clone())
    rw_first.join()
    rw_second.join()
    rw_third.join()
    rw_fourth.join()
    var rw_guard = rw.get().read()
    if(*rw_guard.get() != 4000) {
        return 1
    }
    return 0
}
