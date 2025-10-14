if(def.windows){

@dllimport @stdcall @extern public func CreateThread(lpThreadAttributes:*void,dwStackSize:ulong,lpStartAddress:*void,lpParameter:*void,dwCreationFlags:ulong,lpThreadId:*mut ulong):DWORD;
@dllimport @stdcall @extern public func Sleep(ms:ulong):void


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
@extern public func pthread_join(thread:usize,retval:*mut*void):int;
@extern public func usleep(usec:int):int

@extern public func sysconf(name: int): long
const _SC_NPROCESSORS_ONLN = 84 // Linux/macOS constant for online CPUs

}

public namespace std {

    public namespace concurrent {

        public func hardware_threads() : usize {
            comptime if (def.windows) {
                var info : SYSTEM_INFO
                GetSystemInfo(&info)
                return info.dwNumberOfProcessors as usize
            } else {
                var n = sysconf(_SC_NPROCESSORS_ONLN)
                return if (n < 1) 1 else n as usize
            }
        }
        public func sleep_ms(ms: ulong) {
            comptime if (def.windows) {
                Sleep(ms as u32)
            } else {
                usleep((ms * 1000) as int)
            }
        }

        func spawn_native(entry:(arg : *void) => *void,arg:*void) : u32 {
            comptime if(def.windows){
                var tid:ulong=0u;
                return CreateThread(null,0u,entry,arg,0u,&mut tid)
            } else {
                var th:int=0u;
                if(pthread_create(&mut th,null,entry,arg)!=0){
                    panic("pthread_create")
                }
                return th as u32
            }
        }
        func join_native(h:u32){
            comptime if(def.windows){
                WaitForSingleObject(h as HANDLE, 0xFFFFFFFFu);
                CloseHandle(h as HANDLE)
            } else {
                pthread_join(h,null)
            }
        }

        public struct Thread {
            var handle : u32
            func join(&self) {
                join_native(handle)
            }
        }

        public func spawn(entry:(arg : *void) => *void, arg:*void) : Thread {
            return Thread {
                handle : spawn_native(entry, arg)
            }
        }

        public struct Promise<T> {
            var ready:bool;
            var val:T;
            var m:std.mutex;
            var cv:std.condvar;
            var future_dropped: bool;

            @constructor func constructor(){
                ready=false;
                future_dropped = false
                // TODO: constructor already calls default constructors
                // TODO: not removing because llvm backend might not
                m=std.mutex();
                cv=std.CondVar()
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
                p=pp
            }
            func get(&mut self):T {
                p.m.lock();
                while(!p.ready){ p.cv.wait(p.m) }
                // mark that consumer will free (so worker won't free)
                p.future_dropped = true;
                p.m.unlock();
                // TODO: create a copy here
                var pp = p
                var r = pp.val;
                delete p;
                p=null;
                return r
            }
            @delete
            func delete(&self) {
                if (p != null) {
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

        public struct Task{
            var f:std.function<() => void>
        }

        func worker_main(arg:*void) : *void {
            var P=arg as *mut PoolData;
            while(true){
                var opt:std.Option<Task> = std.Option.None<Task>();
                P.m.lock();
                while(P.q.size()==0u && P.run) {
                    P.cv.wait(P.m)
                }
                if(!P.run && P.q.size()==0u) {
                    P.m.unlock();
                    break
                }
                if(P.q.size()>0u){
                    var t=P.q.take_last();
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


        public struct PoolData {

            var m:std.mutex;
            var cv:std.condvar;
            var q:std.vector<Task>;
            var workers:std.vector<usize>;
            var run:bool

            public func <T> submit(&self, f:std.function<() => T>):Future<T>{
                var prom=malloc(sizeof(Promise<T>)) as *mut Promise<T>;
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

            public func submit_void(&self, f:std.function<() => void>){
                m.lock();
                q.push_back(Task{ f:f });
                cv.notify_one();
                m.unlock()
            }

            @delete
            func delete(&self) {
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
            }

        }

        // just a wrapper around pool data that automatically
        // frees the thread pool
        public struct ThreadPool {

            private var data : *mut PoolData

            public func <T> submit(&self, f:std.function<() => T>):Future<T> {
                return data.submit<T>(f)
            }

            public func submit_void(&self, f:std.function<() => void>) {
                data.submit_void(f)
            }

            @delete
            func delete(&self) {
                delete data;
            }

        }

        public func create_pool(n:uint) : ThreadPool {
            if(n==0u){ panic("n==0") }
            var p=malloc(sizeof(PoolData)) as *mut PoolData;
            new(p) PoolData {
                m:std.mutex(),
                cv:std.CondVar(),
                q:std.vector<Task>(),
                workers:std.vector<usize>(),
                run:true
            };
            var i:uint=0u;
            while(i<n){
                var h=spawn_native(worker_main, p as *void);
                p.workers.push_back(h);
                i=i+1u
            }
            return ThreadPool { data : p }
        }

    }

}