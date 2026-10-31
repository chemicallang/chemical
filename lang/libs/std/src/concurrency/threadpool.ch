if(def.windows){

@dllimport @stdcall @extern public func CreateThread(lpThreadAttributes:*void,dwStackSize:ulong,lpStartAddress:*void,lpParameter:*void,dwCreationFlags:ulong,lpThreadId:*mut ulong):HANDLE;

export Sleep;

public struct SYSTEM_INFO {
    var dwOemId: u32
    var dwPageSize: u32
    var lpMinimumApplicationAddress: *void
    var lpMaximumApplicationAddress: *void
    var dwActiveProcessorMask: usize
    var dwNumberOfProcessors: u32
    var dwProcessorType: u32
    var dwAllocationGranularity: u32
    var wProcessorLevel: u16
    var wProcessorRevision: u16
}

@dllimport @stdcall @extern public func GetSystemInfo(lpSystemInfo: *SYSTEM_INFO): void


} else {

@extern public func pthread_create(thread_out:*usize,attr:*void,start_routine:*void,arg:*void):int;
@extern public func pthread_attr_init(attr:*mut u8):int;
@extern public func pthread_attr_setstacksize(attr:*mut u8,size:size_t):int;
@extern public func pthread_attr_destroy(attr:*mut u8):int;
@extern public func pthread_join(thread:usize,retval:*mut*void):int;
@extern public func usleep(usec:int):int

@extern public func sysconf(name: int): long
const _SC_NPROCESSORS_ONLN = 84 // Linux/macOS constant for online CPUs

// large enough for pthread_attr_t on glibc (56) and musl (4)
comptime const PTHREAD_ATTR_T_SIZE = 64

}

public namespace std {

    public namespace concurrent {

        public func hardware_threads() : usize {
            comptime if (def.windows) {
                var info = zeroed<SYSTEM_INFO>()
                GetSystemInfo(&raw info)
                return info.dwNumberOfProcessors as usize
            } else {
                var n = unsafe(sysconf(_SC_NPROCESSORS_ONLN))
                return if (n < 1) 1 as usize else n as usize
            }
        }
        public func sleep_ms(ms: ulong) {
            comptime if (def.windows) {
                Sleep(ms as u32)
            } else {
                unsafe { usleep((ms * 1000) as int) }
            }
        }

        unsafe func spawn_native(entry:(arg : *void) => *void,arg:*void) : usize {
            comptime if(def.windows){
                var tid:ulong=0u;
                return CreateThread(null,0u,entry,arg,0u,&raw mut tid) as usize
            } else {
                var th:usize=0u;
                // musl defaults to a 128KB thread stack when RLIMIT_STACK is
                // unlimited, which overflows with deep frames + posix_spawn's
                // 5KB buffer. Give workers an explicit 8MB stack like glibc.
                var attr_storage:[PTHREAD_ATTR_T_SIZE / 8]ulong
                const attr_ptr = &raw mut attr_storage[0] as *mut u8
                if(unsafe(pthread_attr_init(attr_ptr))!=0){
                    panic("pthread_attr_init")
                }
                unsafe { pthread_attr_setstacksize(attr_ptr, (8 * 1024 * 1024) as size_t) }
                if(unsafe(pthread_create(&raw mut th,attr_ptr,entry,arg))!=0){
                    panic("pthread_create")
                }
                unsafe { pthread_attr_destroy(attr_ptr) }
                return th
            }
        }
        func join_native(h:usize){
            comptime if(def.windows){
                WaitForSingleObject(h as HANDLE, 0xFFFFFFFFu);
                CloseHandle(h as HANDLE)
            } else {
                unsafe { pthread_join(h,null) }
            }
        }

        public struct Thread {
            // The native handle is unverified; safe code must not forge a
            // thread and call join on an arbitrary value.
            private var handle : usize
            private var valid : bool

            public unsafe func from_raw(h : usize) : Thread {
                return Thread { handle : h, valid : true }
            }

            public func join(&mut self) {
                if(!valid) {
                    panic("thread handle is invalid or already joined")
                }
                valid = false
                join_native(handle)
            }
        }

        // Joins through a raw handle. The pointer must name a live Thread; the
        // scoped implementation is the only in-tree caller and keeps it alive
        // until this returns.
        public unsafe func join_thread(thread : *mut Thread) {
            unsafe { (*thread).join() }
        }

        // Moving a raw entry/argument pair to another thread is only sound
        // when the caller has proved the pointee outlives the thread and that
        // the entry reaches no non-Send state. The compiler cannot check that
        // through `*void`, so thread creation is an explicit unsafe boundary
        // until scoped/Send-typed threads land (M6.3).
        public unsafe func spawn(entry:(arg : *void) => *void, arg:*void) : Thread {
            return unsafe(Thread.from_raw(unsafe(spawn_native(entry, arg))))
        }

        public struct Promise<T> {
            var ready:bool;
            var val:T;
            var m:std.mutex;
            var cv:std.condvar;
            var future_dropped: bool;

            @constructor func constructor(){
                var xx : T
                return Promise<T> {
                    ready=false;
                    future_dropped = false
                    // TODO: poor way to initialize
                    val : xx
                    m=std.mutex()
                    cv=std.CondVar()
                }
            }

            func set(&mut self,x:T){
                m.lock();
                val=x;
                ready=true;
                cv.notify_all();
                m.unlock()
            }
        }

        public struct Future<T>{
            var p:*mut Promise<T>
            @constructor func constructor(pp:*mut Promise<T>) {
                return Future<T> { p : pp }
            }
            func get(&mut self):T {
                var thing : T
                unsafe {
                    p.m.lock();
                    while(!p.ready){ p.cv.wait(&mut p.m) }
                    // mark that consumer will free (so worker won't free)
                    p.future_dropped = true;
                    p.m.unlock();
                    // TODO: create a copy here
                    var pp = p
                    var r = std::replace(&mut pp.val, thing);
                    delete p;
                    p=null;
                    return r
                }
            }
            @delete
            func delete(&mut self) {
                if (p != null) {
                    unsafe {
                        p.m.lock();
                        if (!p.ready) {
                            // promise not set yet: mark that future is dropped
                            p.future_dropped = true;
                            p.m.unlock();
                            // do not free now — worker will free after set
                        } else {
                            // promise already set — consumer can free it now
                            p.future_dropped = true;
                            p.m.unlock();
                            delete p;
                            p = null;
                        }
                    }
                }
            }
        }

        public struct Task{
            var f:std.function<() => void>
        }

        func worker_main(arg:*void) : *void {
        unsafe {
                var P=arg as *mut PoolData;
                while(true){
                    var opt:std.Option<Task> = std.Option.None<Task>();
                    P.m.lock();
                    while(P.q.size()==0u && P.run) {
                        P.cv.wait(&mut P.m)
                    }
                    if(!P.run && P.q.size()==0u) {
                        P.m.unlock();
                        break
                    }
                    if(P.q.size()>0u){
                        var first_ele = P.q.get_ptr(0);
                        // move the task out by replacing it with a dummy, then remove
                        var t = std::replace(unsafe(&mut *first_ele), Task { f : ||() => {} });
                        P.q.remove(0);
                        opt = std.Option.Some<Task>(t)
                    }
                    P.m.unlock();
                    if(opt is std.Option.Some){
                        var Some(tt)=opt else unreachable;
                        tt.f()
                    }
                }
                return null

        }
    }


        public struct PoolData {

            var m:std.mutex;
            var cv:std.condvar;
            var q:std.vector<Task>;
            var workers:std.vector<usize>;
            var run:bool

            public unsafe func <T> submit(&mut self, f:std.function<() => T>):Future<T>{
        unsafe {
                    var prom=unsafe(malloc(sizeof(Promise<T>))) as *mut Promise<T>;
                    if(prom == null) {
                        panic("failed to allocate thread pool promise")
                    }
                    new(prom)Promise<T>();
                    m.lock();
                    q.push_back(Task{ f:|f,prom|() => {
                            var r=f();
                            // set value under the Promise's synchronization
                            prom.m.lock();
                            prom.val = r;
                            prom.ready = true;
                            prom.cv.notify_all();
                            var should_free = prom.future_dropped; // check under lock
                            prom.m.unlock();
                            if (should_free) {
                                // consumer already indicated it will not consume -> worker must free
                                delete prom;
                            }
                        }
                    });
                    cv.notify_one();
                    m.unlock();
                    return Future<T>(prom)

        }
    }

            public unsafe func submit_void(&mut self, f:std.function<() => void>){
                m.lock();
                q.push_back(Task{ f:f });
                cv.notify_one();
                m.unlock()
            }

            @delete
            func delete(&mut self) { unsafe {
                m.lock();
                run=false;
                cv.notify_all();
                m.unlock();
                var i=0u;
                while(i<workers.size()){
                    join_native(*workers.get_ptr(i));
                    i=i+1u
                }
                workers.clear();
                q.clear()
             }}

        }

        // just a wrapper around pool data that automatically
        // frees the thread pool
        public struct ThreadPool {

            private var data : *mut PoolData

            public unsafe func <T> submit(&self, f:std.function<() => T>):Future<T> {
                return unsafe(data.submit<T>(f))
            }

            public unsafe func submit_void(&self, f:std.function<() => void>) {
                unsafe { data.submit_void(f) }
            }

            // Explicit-generic forms used by the free safe wrappers below.
            // (A method call cannot infer both `U` and the capture witness `C`
            // from a lambda argument yet, so the inference-friendly entry points
            // are the free functions in `std::concurrent`.)
            public func <U, C> submit_closure_explicit(&self, f : %capture<() => U, std::default_function_instance, C>) : Future<U> where C : Send + 'static, U : Send {
                return unsafe(data.submit<U>(f))
            }

            @delete
            func delete(&self) {
                unsafe { delete data; }
            }

            // The pool owns the raw allocation; construction is unchecked.
            public unsafe func from_raw(data : *mut PoolData) : ThreadPool {
        unsafe {
                    return ThreadPool { data : data }

        }
    }

        }

        // Safe capturing submission. The capture witness proves the environment
        // is Send and contains no borrowed stack storage; the task is owned by
        // the pool until it runs.
        public func <U, C> submit_closure(f : %capture<() => U, std::default_function_instance, C>, pool : &ThreadPool) : Future<U> where C : Send + 'static, U : Send {
            return pool.submit_closure_explicit<U, C>(f)
        }

        public func create_pool(n:uint) : ThreadPool {
        unsafe {
                if(n==0u){ panic("n==0") }
                var p=unsafe(malloc(sizeof(PoolData))) as *mut PoolData;
                if(p == null) {
                    panic("failed to allocate thread pool")
                }
                new(p) PoolData {
                    m:std.mutex(),
                    cv:std.CondVar(),
                    q:std.vector<Task>(),
                    workers:std.vector<usize>(),
                    run:true
                };
                var i:uint=0u;
                while(i<n){
                    var h=unsafe(spawn_native(worker_main, p as *void));
                    p.workers.push_back(h);
                    i=i+1u
                }
                return unsafe(ThreadPool.from_raw(p))

        }
    }

    }

    public namespace thread {

        @direct_init
        public struct ResultCell<T> {
            internal var value : std::Option<T>
        }

        @direct_init
        public struct SpawnData<T, U> {
            internal var f : (value : T) => U
            internal var arg : std::ManuallyDrop<T>
            internal var result : *mut ResultCell<U>
        }

        @direct_init
        public struct VoidSpawnData<T> {
            internal var f : (value : T) => void
            internal var arg : std::ManuallyDrop<T>
        }

        // Capturing closure state. `C` is the capture-environment witness; it
        // is not stored separately because the erased instance already owns the
        // environment. The witness makes the `C: Send + 'static` bound at the
        // safe entry inspect the values the closure actually captured.
        @direct_init
        public struct ClosureSpawnData<U> {
            internal var f : %capture<() => U, std::default_function_instance>
            internal var result : *mut ResultCell<U>
        }

        @direct_init
        public struct VoidClosureSpawnData {
            internal var f : %capture<() => void, std::default_function_instance>
        }

        @direct_init
        public struct ScopedClosureSpawnData {
            internal var f : %capture<() => void, std::default_function_instance>
        }

        // A capturing scoped closure that also receives a borrowed value. The
        // capture witness stays on the call-site parameter; the stored field is
        // erased because the worker is joined before the scope returns.
        @direct_init
        public struct ScopedClosureValueData<T> {
            internal var f : %capture<(value : &T) => void, std::default_function_instance>
            internal var value : *T
        }

        @direct_init
        public struct ScopedClosureMutData<T> {
            internal var f : %capture<(value : &mut T) => void, std::default_function_instance>
            internal var value : *mut T
        }

        func <T, U> thread_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut SpawnData<T, U>
                var value = data.arg.take()
                var result = data.f(value)
                data.result.value = std::Option.Some<U>(result)
                delete data
                return null
            }
        }

        func <T> void_thread_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut VoidSpawnData<T>
                var value = data.arg.take()
                data.f(value)
                delete data
                return null
            }
        }

        func <U> closure_thread_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut ClosureSpawnData<U>
                var result = data.f()
                data.result.value = std::Option.Some<U>(result)
                delete data
                return null
            }
        }

        func void_closure_thread_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut VoidClosureSpawnData
                data.f()
                delete data
                return null
            }
        }

        func scoped_closure_thread_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut ScopedClosureSpawnData
                data.f()
                delete data
                return null
            }
        }

        func <T> scoped_closure_value_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut ScopedClosureValueData<T>
                data.f(&*data.value)
                delete data
                return null
            }
        }

        func <T> scoped_closure_mut_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut ScopedClosureMutData<T>
                data.f(&mut *data.value)
                delete data
                return null
            }
        }

        public struct JoinHandle<T> {
            internal var thread : std::concurrent.Thread
            internal var result : *mut ResultCell<T>
            internal var joined : bool

            public func join(&mut self) : T {
                if(!joined) {
                    thread.join()
                    joined = true
                }
                if(result == null) {
                    panic("thread result is already taken")
                }
                var out = unsafe((*result).value.take())
                unsafe { delete result }
                result = null
                return out
            }

            @delete
            func delete(&mut self) {
                if(!joined) {
                    thread.join()
                    joined = true
                }
                if(result != null) {
                    unsafe { delete result }
                    result = null
                }
            }
        }

        unsafe impl<T> Send for JoinHandle<T> where T: Send { }
        unsafe impl<T> Sync for JoinHandle<T> where T: Send { }

        // Both parameters are explicit until return-type inference from a
        // function pointer lands: `spawn<int, int>(worker, 41)`.
        public func <T, U> spawn(f : (value : T) => U, arg : T) : JoinHandle<U> where T : Send + 'static, U : Send + 'static {
            var cell = unsafe(malloc(sizeof(ResultCell<U>))) as *mut ResultCell<U>
            if(cell == null) {
                panic("failed to allocate thread result")
            }
            unsafe { new(cell) ResultCell<U> { value : std::Option.None<U>() } }
            var data = unsafe(malloc(sizeof(SpawnData<T, U>))) as *mut SpawnData<T, U>
            if(data == null) {
                unsafe { free(cell) }
                panic("failed to allocate thread state")
            }
            unsafe {
                new(data) SpawnData<T, U> {
                    f : f,
                    arg : std::ManuallyDrop<T> { value : arg },
                    result : cell
                }
            }
            var t = unsafe(std::concurrent.spawn(thread_entry<T, U>, data as *void))
            return JoinHandle<U> {
                thread : t,
                result : cell,
                joined : false
            }
        }

        public func <T> spawn_void(f : (value : T) => void, arg : T) : std::concurrent.Thread where T : Send + 'static {
            var data = unsafe(malloc(sizeof(VoidSpawnData<T>))) as *mut VoidSpawnData<T>
            if(data == null) {
                panic("failed to allocate thread state")
            }
            unsafe {
                new(data) VoidSpawnData<T> {
                    f : f,
                    arg : std::ManuallyDrop<T> { value : arg }
                }
            }
            return unsafe(std::concurrent.spawn(void_thread_entry<T>, data as *void))
        }

        // Safe capturing task with an owned capture environment. `C: Send + 'static`
        // means every captured value is transferable and contains no borrowed
        // stack storage; a by-reference or Rc capture is rejected here.
        public func <U, C> spawn_closure(f : %capture<() => U, std::default_function_instance, C>) : JoinHandle<U> where C : Send + 'static, U : Send + 'static {
            var cell = unsafe(malloc(sizeof(ResultCell<U>))) as *mut ResultCell<U>
            if(cell == null) {
                panic("failed to allocate thread result")
            }
            unsafe { new(cell) ResultCell<U> { value : std::Option.None<U>() } }
            var data = unsafe(malloc(sizeof(ClosureSpawnData<U>))) as *mut ClosureSpawnData<U>
            if(data == null) {
                unsafe { free(cell) }
                panic("failed to allocate thread state")
            }
            unsafe {
                new(data) ClosureSpawnData<U> {
                    f : f,
                    result : cell
                }
            }
            var t = unsafe(std::concurrent.spawn(closure_thread_entry<U>, data as *void))
            return JoinHandle<U> {
                thread : t,
                result : cell,
                joined : false
            }
        }

        public func <C> spawn_void_closure(f : %capture<() => void, std::default_function_instance, C>) : std::concurrent.Thread where C : Send + 'static {
            var data = unsafe(malloc(sizeof(VoidClosureSpawnData))) as *mut VoidClosureSpawnData
            if(data == null) {
                panic("failed to allocate thread state")
            }
            unsafe {
                new(data) VoidClosureSpawnData {
                    f : f
                }
            }
            return unsafe(std::concurrent.spawn(void_closure_thread_entry, data as *void))
        }

        // A capturing closure whose borrowed captures are confined to this
        // call: the worker is joined before returning. Construction is allowed
        // without `unsafe` only at this audited call boundary; the capture
        // witness must still be Send (so `&T` requires `T: Sync`), but it does
        // not need to be `'static` because the call never returns with the
        // worker alive.
        public func <C> scoped_spawn_closure(f : %capture<() => void, std::default_function_instance, C>) where C : Send {
            var data = unsafe(malloc(sizeof(ScopedClosureSpawnData))) as *mut ScopedClosureSpawnData
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureSpawnData {
                    f : f
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_thread_entry, data as *void))
            t.join()
        }

        // Value-passing form of the capturing scoped closure: the closure has
        // its own capture environment and also receives a shared borrowed
        // value. The worker is joined before returning, so the capture witness
        // only needs `Send` (not `'static`); `T: Sync` is what makes sharing
        // the value with the worker sound.
        public func <T, C> scoped_spawn_closure_value(
            f : %capture<(value : &T) => void, std::default_function_instance, C>,
            value : &T
        ) where C : Send, T : Sync {
            var data = unsafe(malloc(sizeof(ScopedClosureValueData<T>))) as *mut ScopedClosureValueData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureValueData<T> {
                    f : f,
                    value : value as *T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_value_entry<T>, data as *void))
            t.join()
        }

        // Mutable value-passing form. `T: Send` permits transferring mutable
        // access to the worker; the join happens before the borrow ends.
        public func <T, C> scoped_spawn_closure_mut(
            f : %capture<(value : &mut T) => void, std::default_function_instance, C>,
            value : &mut T
        ) where C : Send, T : Send {
            var data = unsafe(malloc(sizeof(ScopedClosureMutData<T>))) as *mut ScopedClosureMutData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureMutData<T> {
                    f : f,
                    value : value as *mut T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_mut_entry<T>, data as *void))
            t.join()
        }

        // Non-generic scoped entries for a parameter declared as
        // `std::scoped_send_function<T>`. The marker proves the capture
        // environment is Send but not 'static; the join below confines the
        // borrow to the caller's frame.
        public func scoped_spawn_closure_erased(
            f : std::scoped_send_function<() => void>
        ) {
            var data = unsafe(malloc(sizeof(ScopedClosureSpawnData))) as *mut ScopedClosureSpawnData
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureSpawnData {
                    f : f
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_thread_entry, data as *void))
            t.join()
        }

        public func <T> scoped_spawn_closure_value_erased(
            f : std::scoped_send_function<(value : &T) => void>,
            value : &T
        ) where T : Sync {
            var data = unsafe(malloc(sizeof(ScopedClosureValueData<T>))) as *mut ScopedClosureValueData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureValueData<T> {
                    f : f,
                    value : value as *T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_value_entry<T>, data as *void))
            t.join()
        }

        public func <T> scoped_spawn_closure_mut_erased(
            f : std::scoped_send_function<(value : &mut T) => void>,
            value : &mut T
        ) where T : Send {
            var data = unsafe(malloc(sizeof(ScopedClosureMutData<T>))) as *mut ScopedClosureMutData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureMutData<T> {
                    f : f,
                    value : value as *mut T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_mut_entry<T>, data as *void))
            t.join()
        }

        @direct_init
        public struct ScopedData<T : Sync> {
            internal var f : (value : &T) => void
            internal var value : *T
        }

        @direct_init
        public struct ScopedMutData<T : Send> {
            internal var f : (value : &mut T) => void
            internal var value : *mut T
        }

        func <T> scoped_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut ScopedData<T>
                data.f(&*data.value)
                delete data
                return null
            }
        }

        func <T> scoped_mut_entry(arg : *void) : *void {
            unsafe {
                var data = arg as *mut ScopedMutData<T>
                data.f(&mut *data.value)
                delete data
                return null
            }
        }

        // A scoped thread that is joined before returning. The shared value
        // outlives the call by construction, and T: Sync makes sharing it with
        // the worker sound. The entry is still a non-capturing function pointer;
        // capturing scoped closures need the M6.5 erased-capture work.
        public func <T> scoped_spawn(f : (value : &T) => void, value : &T) where T : Sync {
            var data = unsafe(malloc(sizeof(ScopedData<T>))) as *mut ScopedData<T>
            if(data == null) {
                panic("failed to allocate scoped thread state")
            }
            unsafe {
                new(data) ScopedData<T> {
                    f : f,
                    value : value as *T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_entry<T>, data as *void))
            t.join()
        }

        // Mutable scoped thread: the borrow is exclusive for the duration of
        // the call, and the thread is joined before it returns. T: Send is the
        // permission to transfer mutable access to the worker.
        public func <T> scoped_spawn_mut(f : (value : &mut T) => void, value : &mut T) where T : Send {
            var data = unsafe(malloc(sizeof(ScopedMutData<T>))) as *mut ScopedMutData<T>
            if(data == null) {
                panic("failed to allocate scoped thread state")
            }
            unsafe {
                new(data) ScopedMutData<T> {
                    f : f,
                    value : value as *mut T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_mut_entry<T>, data as *void))
            t.join()
        }

        // A multi-handle scope over one shared borrowed value. The scope is
        // only constructible inside `scoped_run`; `spawn` accepts no reference
        // argument, so every worker borrows exactly the value whose lifetime is
        // the enclosing call. All handles are joined by the destructor, before
        // `scoped_run` returns. Capturing closures that need extra state still
        // use M6.5, but this covers concurrent shared scoped work without an
        // erased capture.
        @direct_init
        public struct ScopedScope<T : Sync> {
            // Fixed slots keep the scope type independent of the generic
            // vector implementation; 32 concurrent workers is enough for the
            // safe API, and exceeding it is a clear panic instead of silent
            // loss of a handle. The fields are internal so only this library
            // can construct a scope; the only safe entry is `scoped_run`.
            internal var value : *T
            internal var threads : [32]*mut std::concurrent.Thread
            internal var count : size_t

            public func spawn(&mut self, f : (value : &T) => void) {
                if(count >= 32) {
                    panic("scoped thread limit exceeded")
                }
                var data = unsafe(malloc(sizeof(ScopedData<T>))) as *mut ScopedData<T>
                if(data == null) {
                    panic("failed to allocate scoped thread state")
                }
                unsafe {
                    new(data) ScopedData<T> {
                        f : f,
                        value : self.value
                    }
                }
                var t = unsafe(std::concurrent.spawn(scoped_entry<T>, data as *void))
                var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
                if(slot == null) {
                    panic("failed to allocate scoped thread handle")
                }
                unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
                threads[count] = slot
                count = count + 1
            }

            @delete
            func delete(&mut self) {
                var i : size_t = 0
                while(i < count) {
                    var slot = threads[i]
                    unsafe {
                        std::concurrent.join_thread(slot)
                        dealloc slot
                    }
                    i = i + 1
                }
                count = 0
            }
        }

        // Runs the body with a scope whose workers all share `value`. The body
        // receives the only mutable handle to the scope; it cannot construct a
        // second one or extract the raw value pointer. The scope destructor
        // joins every worker before this call returns.
        public func <T> scoped_run(value : &T, body : (scope : &mut ScopedScope<T>) => void) where T : Sync {
            var scope = ScopedScope<T> {
                value : value as *T,
                threads : zeroed<[32]*mut std::concurrent.Thread>(),
                count : 0
            }
            body(&mut scope)
        }

        // A multi-handle scope for heterogeneous capturing closures. Each
        // worker may capture a different set of stack values; the scope joins
        // every worker in its destructor before `scoped_run_closure` returns.
        // The fields are internal and no method detaches a handle, so the
        // captured borrows cannot outlive the enclosing call.
        @direct_init
        public struct ScopedClosureScope {
            internal var threads : [32]*mut std::concurrent.Thread
            internal var count : size_t

            @delete
            func delete(&mut self) {
                var i : size_t = 0
                while(i < count) {
                    var slot = threads[i]
                    unsafe {
                        std::concurrent.join_thread(slot)
                        dealloc slot
                    }
                    i = i + 1
                }
                count = 0
            }
        }

        // Free-function forms are used because a generic method cannot infer
        // the capture witness `C` from a lambda argument yet. They are the only
        // way to add a worker to a `ScopedClosureScope`; the scope fields stay
        // internal and no detach operation exists.
        public func <C> scoped_spawn_closure_in(
            scope : &mut ScopedClosureScope,
            f : %capture<() => void, std::default_function_instance, C>
        ) where C : Send {
            if(scope.count >= 32) {
                panic("scoped closure limit exceeded")
            }
            var data = unsafe(malloc(sizeof(ScopedClosureSpawnData))) as *mut ScopedClosureSpawnData
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureSpawnData {
                    f : f
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_thread_entry, data as *void))
            var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
            if(slot == null) {
                panic("failed to allocate scoped thread handle")
            }
            unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
            scope.threads[scope.count] = slot
            scope.count = scope.count + 1
        }

        public func <T, C> scoped_spawn_closure_value_in(
            scope : &mut ScopedClosureScope,
            f : %capture<(value : &T) => void, std::default_function_instance, C>,
            value : &T
        ) where C : Send, T : Sync {
            if(scope.count >= 32) {
                panic("scoped closure limit exceeded")
            }
            var data = unsafe(malloc(sizeof(ScopedClosureValueData<T>))) as *mut ScopedClosureValueData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureValueData<T> {
                    f : f,
                    value : value as *T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_value_entry<T>, data as *void))
            var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
            if(slot == null) {
                panic("failed to allocate scoped thread handle")
            }
            unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
            scope.threads[scope.count] = slot
            scope.count = scope.count + 1
        }

        public func <T, C> scoped_spawn_closure_mut_in(
            scope : &mut ScopedClosureScope,
            f : %capture<(value : &mut T) => void, std::default_function_instance, C>,
            value : &mut T
        ) where C : Send, T : Send {
            if(scope.count >= 32) {
                panic("scoped closure limit exceeded")
            }
            var data = unsafe(malloc(sizeof(ScopedClosureMutData<T>))) as *mut ScopedClosureMutData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureMutData<T> {
                    f : f,
                    value : value as *mut T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_mut_entry<T>, data as *void))
            var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
            if(slot == null) {
                panic("failed to allocate scoped thread handle")
            }
            unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
            scope.threads[scope.count] = slot
            scope.count = scope.count + 1
        }

        // Marker forms for a heterogeneous scope. The marker is !Send, so it
        // cannot be submitted to an unscoped thread API; these entries only
        // live as long as the scope's join.
        public func scoped_spawn_closure_in_erased(
            scope : &mut ScopedClosureScope,
            f : std::scoped_send_function<() => void>
        ) {
            if(scope.count >= 32) {
                panic("scoped closure limit exceeded")
            }
            var data = unsafe(malloc(sizeof(ScopedClosureSpawnData))) as *mut ScopedClosureSpawnData
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureSpawnData {
                    f : f
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_thread_entry, data as *void))
            var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
            if(slot == null) {
                panic("failed to allocate scoped thread handle")
            }
            unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
            scope.threads[scope.count] = slot
            scope.count = scope.count + 1
        }

        public func <T> scoped_spawn_closure_value_in_erased(
            scope : &mut ScopedClosureScope,
            f : std::scoped_send_function<(value : &T) => void>,
            value : &T
        ) where T : Sync {
            if(scope.count >= 32) {
                panic("scoped closure limit exceeded")
            }
            var data = unsafe(malloc(sizeof(ScopedClosureValueData<T>))) as *mut ScopedClosureValueData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureValueData<T> {
                    f : f,
                    value : value as *T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_value_entry<T>, data as *void))
            var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
            if(slot == null) {
                panic("failed to allocate scoped thread handle")
            }
            unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
            scope.threads[scope.count] = slot
            scope.count = scope.count + 1
        }

        public func <T> scoped_spawn_closure_mut_in_erased(
            scope : &mut ScopedClosureScope,
            f : std::scoped_send_function<(value : &mut T) => void>,
            value : &mut T
        ) where T : Send {
            if(scope.count >= 32) {
                panic("scoped closure limit exceeded")
            }
            var data = unsafe(malloc(sizeof(ScopedClosureMutData<T>))) as *mut ScopedClosureMutData<T>
            if(data == null) {
                panic("failed to allocate scoped closure state")
            }
            unsafe {
                new(data) ScopedClosureMutData<T> {
                    f : f,
                    value : value as *mut T
                }
            }
            var t = unsafe(std::concurrent.spawn(scoped_closure_mut_entry<T>, data as *void))
            var slot = unsafe(malloc(sizeof(std::concurrent.Thread))) as *mut std::concurrent.Thread
            if(slot == null) {
                panic("failed to allocate scoped thread handle")
            }
            unsafe { memcpy(slot, &raw t, sizeof(std::concurrent.Thread)) }
            scope.threads[scope.count] = slot
            scope.count = scope.count + 1
        }

        // Scoped execution with a heterogeneous closure scope. `body` runs on
        // the calling thread and receives the only mutable handle to the scope;
        // every worker spawned by the body is joined when the scope is dropped,
        // before this call returns. The body may capture caller storage because
        // it is not moved to a worker.
        public func <B> scoped_run_closure(
            body : %capture<(scope : &mut ScopedClosureScope) => void, std::default_function_instance, B>
        ) {
            var scope = ScopedClosureScope {
                threads : zeroed<[32]*mut std::concurrent.Thread>(),
                count : 0
            }
            body(&mut scope)
        }
    }

}