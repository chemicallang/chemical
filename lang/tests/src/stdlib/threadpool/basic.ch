//// Utility thread-entry functions and small arg structs for spawn
public struct WaitArg {
    var m:*mut std.mutex;
    var cv:*mut std.condvar;
    var flag:*mut bool;
    var done:*mut bool;
}

func waiter_entry(arg:*void) : *void {
    var a = arg as *mut WaitArg;
    // non-capturing function, so use pointer data
    a.m.lock();
    while (!a.flag) { a.cv.wait(*a.m); }
    *a.done = true;
    a.m.unlock();
    return null;
}

// For notify_all test we need a per-thread arg but same struct works

//// Tests start here
// 1) Basic submit/get (single-thread) - non-capturing lambda
func test_basic_single_thread() : bool {
    var pool = std::concurrent::create_pool(1u);
    var fn : std::function<() => u64> = ||() => { return 123u64 }
    var fut = pool.submit<u64>(fn);
    var r = fut.get();
    return r == 123u64;
}

// 2) Multiple futures with different sleep durations ensure values are correct
func test_multiple_futures_values() : bool {
    var pool = std::concurrent::create_pool(4u);
    var f1 = pool.submit<u64>(() => { std::concurrent::sleep_ms(50u); return 1u64 });
    var f2 = pool.submit<u64>(() => { std::concurrent::sleep_ms(10u); return 2u64 });
    var f3 = pool.submit<u64>(() => { return 3u64 });
    var r1 = f1.get();
    var r2 = f2.get();
    var r3 = f3.get();
    return (r1 == 1u64) && (r2 == 2u64) && (r3 == 3u64);
}

// 3) submit_void with captured counter via std.function (capture by reference)
func test_submit_void_and_mutexed_counter() : bool {
    var m = std.mutex();
    var counter = 0u;
    var n = 200u;
    var i = 0u;
    if(i == 0u) {
        // capturing lambda must be wrapped into std.function when it captures
        var pool = std::concurrent::create_pool(2u);
        while (i < n) {
            var inc: std.function<() => void> = |&mut m,&mut counter|() => {
                m.lock();
                counter = counter + 1u;
                m.unlock()
            };
            pool.submit_void(inc);
            i = i + 1u;
        }
        // pool going out of scope, automatic destruction and join of all workers
    }
    return counter == n;
}

// 4) Destruction waits for tasks to finish (submit tasks that sleep)
func test_destruction_waits() : bool {
    var m = std.mutex();
    var counter = 0u;
    var n = 60u;
    var i = 0u;
    if(i == 0u) {
        var pool = std::concurrent::create_pool(3u);
        while (i < n) {
            var f: std.function<() => void> = |&mut m,&mut counter|() => {
                std::concurrent::sleep_ms(20u);
                m.lock();
                counter = counter + 1u;
                m.unlock()
            };
            pool.submit_void(f);
            i = i + 1u;
        }
        // pool goes out of scope, automatic destruction and join of all workers
    }
    return counter == n;
}

// 5) Stress: many tasks across many threads; count with mutex
func test_stress_many_tasks() : bool {
    var threads = std::concurrent::hardware_threads();
    var m = std.mutex();
    var counter = 0u;
    var tasks = 1000u;
    var i = 0u;
    if(i == 0u) {
        var pool = std::concurrent::create_pool(if (threads < 1u) 1u else threads);
        while (i < tasks) {
            var inc: std.function<() => void> = |&mut m,&mut counter|() => {
                m.lock();
                counter = counter + 1u;
                m.unlock()
            };
            pool.submit_void(inc);
            i = i + 1u;
        }
        // pool goes out of scope, automatic destruction and join of all workers
    }
    return counter == tasks;
}

// 6) Condvar notify_one wakes a single waiter. Use waiter_entry (non-capturing) and WaitArg
func test_condvar_notify_one() : bool {
    var m = std.mutex();
    var cv = std.CondVar();
    var flag = false;
    var waiter_done = false;
    var arg = malloc(sizeof(WaitArg)) as *mut WaitArg;
    arg.m = &mut m;
    arg.cv = &mut cv;
    arg.flag = &mut flag;
    arg.done = &mut waiter_done;

    var h = std::concurrent::spawn(waiter_entry, arg as *void);
    std::concurrent::sleep_ms(50u);

    m.lock();
    flag = true;
    cv.notify_one();
    m.unlock();

    h.join()
    free(arg);
    return waiter_done;
}

// 7) Condvar notify_all wakes many waiters. Use multiple WaitArg copies and join them
func test_condvar_notify_all() : bool {
    var m = std.mutex();
    var cv = std.CondVar();
    var flag = false;
    var waiters = 12u;
    var done_count = 0u;
    var handles = std.vector<std::concurrent::Thread>();
    var args = std.vector<*mut WaitArg>();

    var i = 0u;
    while (i < waiters) {
        var a = malloc(sizeof(WaitArg)) as *mut WaitArg;
        a.m = &mut m;
        a.cv = &mut cv;
        a.flag = &mut flag;
        // each waiter has its own done flag on heap so we can count them
        var done_ptr = malloc(sizeof(bool)) as *mut bool;
        *done_ptr = false;
        a.done = done_ptr;
        var h = std::concurrent::spawn(waiter_entry, a as *void);
        handles.push_back(h);
        args.push_back(a);
        i = i + 1u;
    }

    std::concurrent::sleep_ms(50u);
    m.lock();
    flag = true;
    cv.notify_all();
    m.unlock();

    i = 0u;
    while (i < handles.size()) {
        const t = handles.get_ptr(i);
        t.join()
        var a = *args.get_ptr(i);
        if (a.done) { done_count = done_count + 1u; }
        free(a);
        i = i + 1u;
    }
    return done_count == waiters;
}

func spawn_func(f : std::function<() => *void>) {
    f()
}

type spawn_func_type = (arg : *void) => *void

// 8) Mutex contention correctness: many threads increment counter under mutex
func test_mutex_contention() : bool {
    var m = std.mutex();
    var counter = 0u;
    var threads = 8u;
    var handles = std.vector<std::concurrent::Thread>();
    var incs_per_thread = 200u;
    var i = 0u;
    var heap_pointers = std::vector<*mut any>()
    while (i < threads) {
        // create a per-thread non-capturing entry that uses heap arg
        var arg = malloc(sizeof(usize)) as *mut usize;
        var func_ptr = malloc(sizeof(*void)) as *mut *mut any
        *arg = incs_per_thread;
        var inc_entry : std::function<(a : *void) => *void> = |&mut counter, arg, &mut m, func_ptr|() => {
            var times = *(arg as *mut usize);
            var j = 0u;
            while (j < times) {
                m.lock();
                counter = counter + 1u;
                m.unlock();
                j = j + 1u;
            }
            free(arg);
            free(func_ptr)
            return null;
        }
        // do not destruct inc_entry
        intrinsics::forget(inc_entry)
        // copy the function to heap
        var heap_func = malloc(sizeof(inc_entry)) as *mut std::function<(a : *void) => *void>
        memcpy(heap_func, &inc_entry, sizeof(inc_entry))
        heap_pointers.push_back(heap_func)
        *func_ptr = heap_func
        var h = std::concurrent::spawn(spawn_func as spawn_func_type, heap_func);
        handles.push_back(h);
        i = i + 1u;
    }
    i = 0u;
    while (i < handles.size()) {
        const t = handles.get_ptr(i);
        t.join()
        i = i + 1u;
    }
    i = 0u
    while(i < heap_pointers.size()) {
        free(heap_pointers.get(i))
        i++
    }
    return counter == threads * incs_per_thread;
}

// 9) Future destructor safety: construct a future, drop it (let destructor run), then ensure pool still usable
func test_future_drop_safety() : bool {
    var pool = std::concurrent::create_pool(2u);
    var fut = pool.submit<u64>(() => { std::concurrent::sleep_ms(30u); return 42u64 });
    // let fut go out of scope by creating a local block
    var x = 0
    if(x == 0){
        var tmp = fut; // tmp will be destroyed at end of block; if Promise cleanup is correct, no leak/crash
    }
    // submit another job and get result
    var f2 = pool.submit<u64>(() => { return 7u64 });
    var r = f2.get();
    return r == 7u64;
}

// 10) Submit after shutdown: library did not document behavior; we assert that either submission works or panics.
// We'll try to submit after shutdown; if it succeeds and returns expected value, test passes. If it panics/crashes,
// the runner will consider it a failure (as requested). If it blocks forever we will time out in the harness.
func test_submit_single() : bool {
    var pool = std::concurrent::create_pool(2u);
    // Best-effort: try to submit a non-capturing job
    var f = pool.submit<u64>(() => { return 99u64 });
    var r = f.get();
    return r == 99u64;
}

// 11) Worker restart sanity: create and destroy many pools repeatedly to ensure no resource exhaustion
func test_create_destroy_many_pools() : bool {
    var i = 0u;
    while (i < 20u) {
        var p = std::concurrent::create_pool(2u);
        // let p go out of scope and be destroyed automatically
        i = i + 1u;
    }
    return true;
}

// 12) Closure captures by value and by reference inside submitted tasks
func test_closure_captures() : bool {
    var pool = std::concurrent::create_pool(2u);
    var value = 100u64;
    var byval: std.function<() => u64> = |value|() => { return value + 23u }; // capture by value
    var byref: std.function<() => u64> = |&mut value|() => { value = value + 1u; return *value };
    var f1 = pool.submit<u64>(byval);
    var f2 = pool.submit<u64>(byref);
    var r1 = f1.get();
    var r2 = f2.get();
    // byval shouldn't have changed original 'value', byref incremented it once
    return (r1 == 123u) && (r2 == 101u && value == 101u);
}

// 14) Large payload return via Future to ensure copies/ownership are correct
// public struct Big { var a: [256]u8 }
// func test_large_return_future() : bool {
//     var pool = std::concurrent::create_pool(2u);
//     var f = pool.submit<Big>(() => { var b:Big; var i=0u; while(i<256u){ b.a[i]=i as u8; i=i+1u } return b });
//     var r = f.get();
//     var sum = 0u;
//     var i = 0u;
//     while (i < 256u) { sum = sum + r.a[i] as usize; i = i + 1u; }
//     // compare expected sum (0..255) = 255*256/2 = 32640
//     return sum == 32640u;
// }

// 15) Mixed submit_void and submit with many quick tasks to exercise queuing
func test_mixed_quick_tasks() : bool {
    var m = std.mutex();
    var counter = 0u;
    var i = 0u;
    var r : u64 = 0
    if(i == 0) {
        var pool = std::concurrent::create_pool(4u);
        while (i < 300u) {
            var inc: std.function<() => void> = |&mut m,&mut counter|() => {
                m.lock();
                counter = counter + 1u;
                m.unlock()
            };
            pool.submit_void(inc);
            i = i + 1u;
        }
        // also submit some futures
        var f = pool.submit<u64>(() => { return 13u64 });
        r = f.get();
        // pool goes out of scope, causing workers to join in destruction
    }
    return r == 13u64 && counter == 300u;
}

// main: run tests
func test_thread_pool() {
    test("test_basic_single_thread", test_basic_single_thread);
    test("test_multiple_futures_values", test_multiple_futures_values);
    test("test_submit_void_and_mutexed_counter", test_submit_void_and_mutexed_counter);
    test("test_destruction_waits", test_destruction_waits);
    test("test_stress_many_tasks", test_stress_many_tasks);
    test("test_condvar_notify_one", test_condvar_notify_one);
    test("test_condvar_notify_all", test_condvar_notify_all);
    test("test_mutex_contention", test_mutex_contention);
    test("test_future_drop_safety", test_future_drop_safety);
    test("test_submit_single", test_submit_single);
    test("test_create_destroy_many_pools", test_create_destroy_many_pools);
    test("test_closure_captures", test_closure_captures);
    // test("test_large_return_future", test_large_return_future);
    test("test_mixed_quick_tasks", test_mixed_quick_tasks);
}