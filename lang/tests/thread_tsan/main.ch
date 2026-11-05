// M6.2 data-race smoke test. Run under ThreadSanitizer with
// scripts/thread-tsan.sh; the plain binary is also a normal runtime test.
using namespace std;

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

func increment_once(shared : std::Arc<std::Mutex<int>>) : int {
    var guard = shared.get().lock()
    var value = guard.get()
    *value = *value + 1
    return 0
}

async func async_worker_increment(shared : std::Arc<std::Mutex<int>>) : int {
    var u = await async::yield_now()
    return increment_once(shared)
}

func worker_channel_send(sender : async::Sender<int>) : int {
    var i = 0
    while(i < 200) {
        if(!sender.send(i)) {
            return -1
        }
        i = i + 1
    }
    sender.close()
    return 0
}

func run_threadpool_increments(shared : std::Arc<std::Mutex<int>>) {
    var pool = std::concurrent.create_pool(4u)
    var i = 0
    while(i < 200) {
        var s = shared.clone()
        var task : std::function<() => void> = |s|() => {
            var guard = s.get().lock()
            var value = guard.get()
            *value = *value + 1
        }
        unsafe(pool.submit_void(task))
        i = i + 1
    }
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

    // A cross-thread channel producer and non-blocking consumer must hand off
    // every message without losing or duplicating one.
    var ch = async::channel<int>()
    var producer = ch.sender.clone_sender()
    var producer_thread = std::thread.spawn<async::Sender<int>, int>(
        worker_channel_send, producer)
    var received = 0
    var total = 0
    var spins = 0
    while(received < 200 && spins < 200000) {
        var next = ch.receiver.try_recv()
        if(next is std::Option.Some) {
            var Some(value) = next else unsafe { unreachable }
            total = total + value
            received = received + 1
        } else {
            std::concurrent.sleep_ms(1)
            spins = spins + 1
        }
    }
    producer_thread.join()
    if(received != 200 || total != 19900) {
        return 1
    }

    // The pool queue, worker threads, and promise handoff must be race-free.
    var pool_shared = std::arc(std::new_mutex(0))
    run_threadpool_increments(pool_shared.clone())
    var pool_guard = pool_shared.get().lock()
    if(*pool_guard.get() != 200) {
        return 1
    }

    // The executor queue and cross-thread spawn_send handoff must also be
    // race-free when tasks suspend on a yield.
    var async_shared = std::arc(std::new_mutex(0))
    var async_first = async::spawn_send(async_worker_increment(async_shared.clone()))
    var async_second = async::spawn_send(async_worker_increment(async_shared.clone()))
    var async_first_result = async::block_on_send<int>(async_first)
    var async_second_result = async::block_on_send<int>(async_second)
    if(async_first_result != 0 || async_second_result != 0) {
        return 1
    }
    var async_guard = async_shared.get().lock()
    if(*async_guard.get() != 2) {
        return 1
    }
    return 0
}
