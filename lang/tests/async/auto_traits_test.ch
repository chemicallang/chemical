// S6.0: public async value types carry the right auto traits.
//
// The erased `FutureHandle` is deliberately !Send (its frame may borrow the
// caller); `SendFutureHandle` is the checked cross-thread form. `Waker` and
// `Context` are shared across poll sites, and `Poll<T>` follows its payload.
using namespace std;

func <T> async_needs_send(v : T) : int where T : Send { return 0 }
func <T> async_needs_sync(v : &T) : int where T : Sync { return 0 }
func <T> async_needs_send_type() : int where T : Send { return 0 }
func <T> async_needs_sync_type() : int where T : Sync { return 0 }

@test
func async_public_value_types_are_send_and_sync(env : &mut TestEnv) {
    var ready = core::async::Poll.Ready<int>(7)
    if(async_needs_send(ready) != 0) {
        env.error("Poll<int> should be Send")
    }

    var ctx = core::async::Context {
        waker : core::async::Waker.empty()
    }
    if(async_needs_send(ctx) != 0) {
        env.error("Context should be Send when its waker is Send")
    }

    var ctx2 = core::async::Context {
        waker : core::async::Waker.empty()
    }
    if(async_needs_sync(&ctx2) != 0) {
        env.error("Context should be Sync when its waker is Sync")
    }

    if(async_needs_send_type<core::async::SendFutureHandle<int> >() != 0) {
        env.error("SendFutureHandle<int> should be Send")
    }
    if(async_needs_sync_type<core::async::SendFutureHandle<int> >() != 0) {
        env.error("SendFutureHandle<int> should be Sync")
    }
    if(async_needs_send_type<std::concurrent.Future<int> >() != 0) {
        env.error("thread-pool Future<int> should be Send when its result is Send")
    }
    if(async_needs_send_type<std::concurrent.ThreadPool>() != 0) {
        env.error("ThreadPool should be Send")
    }
    if(async_needs_sync_type<std::concurrent.ThreadPool>() != 0) {
        env.error("ThreadPool should be Sync")
    }

    // A SendFutureHandle is produced only by the checked submission surface;
    // the type-level assertions above cover the erased value form.
    var handle = async::spawn_send<int>(async_ready());
    var result = async::block_on_send<int>(handle)
    if(result != 11) {
        env.error("SendFutureHandle did not carry its result")
    }
}

async func async_ready() : int {
    return 11
}
