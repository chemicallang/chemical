// Windows fd reactor backed by anonymous pipes (CreatePipe) and
// PeekNamedPipe for readiness checks.  This replaces the earlier stub so that
// the tier-0 reactor tests (readable / writable / AsyncFd) pass on Windows.
//
// A full IOCP-backed reactor is Tier 1; this implementation uses busy-polling
// with Sleep(1) which is correct and simple — identical to the POSIX select(2)
// reactor that also does non-blocking readiness probes.

public namespace async {

    public func pipe(fds : *mut int) : int {
        var read_handle : HANDLE = null
        var write_handle : HANDLE = null
        if(!CreatePipe(&raw mut read_handle, &raw mut write_handle, null, 0u as DWORD)) {
            return -1
        }
        fds[0] = _open_osfhandle(read_handle as intptr_t, 0)
        fds[1] = _open_osfhandle(write_handle as intptr_t, 0)
        if(fds[0] == -1 || fds[1] == -1) {
            return -1
        }
        return 0
    }

}
public namespace async {
public namespace posix {

    public struct TimeVal {
        var tv_sec : long
        var tv_usec : long
    }

    public struct PollFd {
        var fd : int
        var events : short
        var revents : short
    }

    comptime const POLLIN : short = 1
    comptime const POLLOUT : short = 4
    comptime const POLLERR : short = 8
    comptime const POLLHUP : short = 16

    public func reactor_is_ready(fd : int, events : short) : bool {
        if(fd < 0) { return false }
        var handle = _get_osfhandle(fd) as HANDLE
        if(handle == null || handle == INVALID_HANDLE_VALUE) { return false }

        var ready = false

        if((events & POLLIN) != 0) {
            var bytes_avail : DWORD = 0
            if(PeekNamedPipe(handle, null, 0u as DWORD, null, &raw mut bytes_avail, null)) {
                if(bytes_avail > 0u as DWORD) {
                    ready = true
                }
            } else {
                ready = true
            }
        }

        if((events & POLLOUT) != 0) {
            ready = true
        }

        return ready
    }

    public func reactor_platform_poll(fds : *mut PollFd, n : ulong, timeout_ms : int) : int {
        var deadline : i64 = 0
        var has_timeout = timeout_ms > 0
        if(has_timeout) {
            deadline = std::now_milli() + (timeout_ms as i64)
        }

        loop {
            var ready_count = 0
            var i = 0u
            while(i < n) {
                fds[i].revents = 0
                var handle = _get_osfhandle(fds[i].fd) as HANDLE
                if(handle != null && handle != INVALID_HANDLE_VALUE) {

                    if((fds[i].events & POLLIN) != 0) {
                        var bytes_avail : DWORD = 0
                        if(PeekNamedPipe(handle, null, 0u as DWORD, null, &raw mut bytes_avail, null)) {
                            if(bytes_avail > 0u as DWORD) {
                                fds[i].revents = fds[i].revents | POLLIN
                            }
                        } else {
                            fds[i].revents = fds[i].revents | POLLIN | POLLHUP
                        }
                    }

                    if((fds[i].events & POLLOUT) != 0) {
                        fds[i].revents = fds[i].revents | POLLOUT
                    }

                } else {
                    fds[i].revents = fds[i].revents | POLLERR
                }

                if(fds[i].revents != 0) {
                    ready_count = ready_count + 1
                }
                i = i + 1u
            }

            if(ready_count > 0) {
                return ready_count
            }

            if(!has_timeout) {
                return 0
            }
            if(std::now_milli() >= deadline) {
                return 0
            }
            Sleep(1u as DWORD)
        }
    }

}
}
