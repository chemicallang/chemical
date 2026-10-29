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
    }

}