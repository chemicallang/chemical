// M6/S6.0: channel handles are audited Send/Sync values. The handle fields are
// internal, so safe code cannot forge one around an arbitrary pointer.
using namespace std;

func <T> channel_needs_send(v : T) : int where T: Send { return 0 }
func <T> channel_needs_sync(v : &T) : int where T: Sync { return 0 }

@test
func channel_handles_are_send_and_sync(env : &mut TestEnv) {
    var ch = async::channel<int>()
    var extra = ch.sender.clone_sender()
    if(channel_needs_send(extra) != 0) {
        env.error("Sender<int> should be Send when the payload is Send")
    }
    if(channel_needs_sync(&ch.receiver) != 0) {
        env.error("Receiver<int> should be Sync when the payload is Send")
    }
    ch.sender.send(7)
    var got = ch.receiver.try_recv()
    if(got is Option.None) {
        env.error("channel lost the sent value")
        return
    }
    var Some(value) = got else unreachable
    if(value != 7) {
        env.error("channel delivered the wrong value")
    }
}

@test
func channel_future_outlives_channel_handles(env : &mut TestEnv) {
    var future : core::async::FutureHandle<int>
    {
        var ch = async::channel<int>()
        var sender = ch.sender
        future = ch.receiver.recv_or(99)
        sender.send(42)
    }
    var value = async::block_on<int>(future)
    if(value != 42) {
        env.error("channel receive future did not keep the channel shared state alive")
    }
}
