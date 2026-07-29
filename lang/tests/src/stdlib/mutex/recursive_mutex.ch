func test_recursive_lock_basic() : bool {
    var m = std::recursive_mutex()
    m.lock()
    var i = 0
    while(i < 10) {
        m.lock()
        m.unlock()
        i += 1
    }
    m.unlock()
    return true
}

func test_recursive_try_lock() : bool {
    var m = std::recursive_mutex()
    if(!m.try_lock()) { return false }
    if(!m.try_lock()) { return false }
    m.unlock()
    m.unlock()
    return true
}

func test_recursive_lock_guard() : bool {
    var m = std::recursive_mutex()
    var val = 0
    {
        var g = std::recursive_lock_guard(&mut m)
        val = 1
        {
            var g2 = std::recursive_lock_guard(&mut m)
            val = 2
        }
    }
    return val == 2
}

func test_recursive_mutex_concurrent() : bool {
    var num = std::concurrent::hardware_threads()
    if(num < 2u) { num = 2u }
    if(num > 4u) { num = 4u }
    var m = std::recursive_mutex()
    var counter = 0u
    var result = false
    var dummy = 0u
    if(dummy == 0u) {
        var pool = std::concurrent::create_pool(num)
        var i = 0u
        while(i < num) {
            var task : std::function<() => void> = |&mut m, &mut counter|() => {
                m.lock()
                *counter = *counter + 1u
                std::concurrent::sleep_ms(1u)
                m.lock()
                *counter = *counter + 1u
                m.unlock()
                m.unlock()
            }
            pool.submit_void(task)
            i += 1u
        }
    }
    return counter == num * 2u
}

func test_recursive_mutex() {
    test("recursive_mutex basic recursive locking (10x nested)", test_recursive_lock_basic)
    test("recursive_mutex try_lock recursive", test_recursive_try_lock)
    test("recursive_lock_guard RAII (nested guards)", test_recursive_lock_guard)
    test("recursive_mutex concurrent access", test_recursive_mutex_concurrent)
}
