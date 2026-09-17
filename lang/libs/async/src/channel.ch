// A single-consumer, multi-producer channel (design M6).
//
// `send` is synchronous (locks, enqueues, wakes the receiver). `recv_or`
// returns an awaitable that becomes ready with the next value, or with the
// caller's `fallback` once every sender has been dropped/closed. A bare
// `recv()` returning `Option<T>` is not possible yet: a vtable parameterized by
// a composite generic (`FutureTable<Option<T>>`) is unsupported (B20).
//
// The queue is a heap linked list rather than `vector<Option<T>>`: a
// `vector<Composite<T>>` is itself unusable inside generic code (B20), and the
// payload is moved out with `memcpy` (B18).
public namespace async {

public struct ChannelNode<T> {
    var value : T
    var next : *mut ChannelNode<T>
}

public struct ChannelShared<T> {
    var m : std::mutex
    var head : *mut ChannelNode<T>
    var tail : *mut ChannelNode<T>
    var closed : bool
    var senders : int
    var receivers : int
    var waker : core::async::Waker
}

// Move a `T` out of a heap node without running a destructor. The node is then
// freed with `dealloc` (plain struct, no destructor).
@retained
public func <T> channel_node_take(n : *mut ChannelNode<T>) : T {
    var temp : T
    unsafe {
        memcpy(&raw mut temp, &raw mut n.value, sizeof(T))
        return temp
    }
}

// Move the payload out of an `Option<T>` through a pointer, leaving `None`.
@retained
public func <T> option_take(o : *mut std::Option<T>) : T {
    var temp : T
    unsafe {
        var Some(value) = *o else unreachable
        memcpy(&raw mut temp, &raw value, sizeof(T))
        new(o) std::Option.None<T>()
        return temp;
    }
}

@retained
public func <T> channel_wake_locked(s : *mut ChannelShared<T>) {
    if(s.waker.vtbl == null) {
        return
    }
    var w = s.waker.clone()
    s.m.unlock()
    w.wake()
    s.m.lock()
}

@retained
public func <T> channel_release_shared(s : *mut ChannelShared<T>) {
    var free_it = false
    s.m.lock()
    if(s.senders <= 0 && s.receivers <= 0) {
        free_it = true
    }
    s.m.unlock()
    if(free_it) {
        unsafe {
            delete s
        }
    }
}

@retained
public func <T> channel_push(s : *mut ChannelShared<T>, value : T) : bool {
    s.m.lock()
    if(s.closed) {
        s.m.unlock()
        return false
    }
    var node = malloc(sizeof(ChannelNode<T>)) as *mut ChannelNode<T>
    new(node) ChannelNode<T> {
        value : value,
        next : null
    }
    if(s.tail == null) {
        s.head = node
        s.tail = node
    } else {
        s.tail.next = node
        s.tail = node
    }
    channel_wake_locked<T>(s)
    s.m.unlock()
    return true
}

@retained
public func <T> channel_pop_opt(s : *mut ChannelShared<T>) : std::Option<T> {
    s.m.lock()
    if(s.head == null) {
        s.m.unlock()
        return std::Option.None<T>()
    }
    var n = s.head
    s.head = n.next
    if(s.head == null) {
        s.tail = null
    }
    s.m.unlock()
    var v = channel_node_take<T>(n)
    unsafe {
        dealloc n
    }
    return std::Option.Some<T>(v)
}

// ---- sender ----------------------------------------------------------------

@direct_init
public struct Sender<T> {
    var shared : *mut ChannelShared<T>

    public func send(&self, value : T) : bool {
        return channel_push<T>(self.shared, value)
    }

    // A new handle to the same channel. The channel closes once every handle
    // (original and clones) has been dropped or `close`d.
    public func clone_sender(&self) : Sender<T> {
        var s = self.shared
        s.m.lock()
        s.senders = s.senders + 1
        s.m.unlock()
        return Sender<T> { shared : s }
    }

    public func close(&self) {
        var s = self.shared
        s.m.lock()
        var was = s.closed
        s.closed = true
        if(!was) {
            channel_wake_locked<T>(s)
        }
        s.m.unlock()
    }

    @delete
    func delete(&mut self) {
        if(self.shared != null) {
            var s = self.shared
            s.m.lock()
            s.senders = s.senders - 1
            if(s.senders <= 0 && !s.closed) {
                s.closed = true
                channel_wake_locked<T>(s)
            }
            s.m.unlock()
            channel_release_shared<T>(s)
            self.shared = null
        }
    }
}

// ---- receiver --------------------------------------------------------------

@direct_init
public struct Receiver<T> {
    var shared : *mut ChannelShared<T>

    // Non-blocking receive. Returns `None` when nothing is queued (the channel
    // may still be open or already be closed).
    public func try_recv(&self) : std::Option<T> {
        return channel_pop_opt<T>(self.shared)
    }

    public func is_closed(&self) : bool {
        var s = self.shared
        s.m.lock()
        var c = s.closed
        s.m.unlock()
        return c
    }

    public func close(&self) {
        var s = self.shared
        s.m.lock()
        s.closed = true
        channel_wake_locked<T>(s)
        s.m.unlock()
    }

    // Await the next value; when every sender is gone (and the queue is empty)
    // become ready with `fallback` instead.
    public func recv_or(&self, fallback : T) : core::async::FutureHandle<T> {
        var st = malloc(sizeof(RecvState<T>)) as *mut RecvState<T>
        var vtbl = malloc(sizeof(core::async::FutureTable<T>)) as *mut core::async::FutureTable<T>
        vtbl.poll = recv_poll<T>
        vtbl.drop = recv_drop<T>
        new(st) RecvState<T> {
            shared : self.shared,
            fallback : std::Option.Some<T>(fallback),
            vtbl : vtbl
        }
        return core::async::FutureHandle<T> { frame : st as *mut void, vtbl : vtbl }
    }

    @delete
    func delete(&mut self) {
        if(self.shared != null) {
            var s = self.shared
            s.m.lock()
            s.receivers = s.receivers - 1
            s.m.unlock()
            channel_release_shared<T>(s)
            self.shared = null
        }
    }
}

public struct RecvState<T> {
    var shared : *mut ChannelShared<T>
    var fallback : std::Option<T>
    var vtbl : *mut core::async::FutureTable<T>
}

@retained
public func <T> recv_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<T> {
    var st = frame as *mut RecvState<T>
    var s = st.shared
    var opt = channel_pop_opt<T>(s)
    if(opt is std::Option.Some) {
        var v = option_take<T>(&raw mut opt)
        return core::async::Poll.Ready<T>(v)
    }
    s.m.lock()
    if(s.closed) {
        var fb = st.fallback.take()
        s.m.unlock()
        return core::async::Poll.Ready<T>(fb)
    }
    if(cx.waker.vtbl != null) {
        s.waker = cx.waker.clone()
    }
    s.m.unlock()
    return core::async::Poll.Pending<T>()
}

@retained
public func <T> recv_drop(frame : *mut void) {
    var st = frame as *mut RecvState<T>
    var vtbl = st.vtbl
    unsafe {
        delete st
    }
    if(vtbl != null) {
        unsafe {
            dealloc vtbl
        }
    }
}

// ---- channel ---------------------------------------------------------------

@direct_init
public struct Channel<T> {
    var sender : Sender<T>
    var receiver : Receiver<T>
}

public func <T> channel() : Channel<T> {
    var s = malloc(sizeof(ChannelShared<T>)) as *mut ChannelShared<T>
    new(s) ChannelShared<T> {
        m : std::mutex(),
        head : null,
        tail : null,
        closed : false,
        senders : 1,
        receivers : 1,
        waker : core::async::Waker { data : null, vtbl : null }
    }
    return Channel<T> {
        sender : Sender<T> { shared : s },
        receiver : Receiver<T> { shared : s }
    }
}

// Clone the sender into a value the caller owns (e.g. to capture in a lambda).
// Use `clone_sender` on the channel's sender rather than moving it out of the
// `Channel<T>`: moving a destructible field out of a value is rejected by the
// move checker.
public func <T> clone_sender(s : *mut ChannelShared<T>) : Sender<T> {
    s.m.lock()
    s.senders = s.senders + 1
    s.m.unlock()
    return Sender<T> { shared : s }
}

}
