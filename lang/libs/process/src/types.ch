public namespace process {

using std::Option;
using std::Result;
using std::string;
using std::string_view;
using std::vector;

public variant ProcessError {
    InvalidArgs(msg : string)
    OperationFailed(msg : string)
    NotRunning()
    TimedOut()
    IoError(msg : string)

    func message(&self) : string {
        switch(self) {
            InvalidArgs(msg) => {
                var s = string("ProcessError: invalid args: ")
                s.append_view(msg.to_view())
                return s
            }
            OperationFailed(msg) => {
                var s = string("ProcessError: operation failed: ")
                s.append_view(msg.to_view())
                return s
            }
            NotRunning() => return string("ProcessError: process is not running")
            TimedOut() => return string("ProcessError: operation timed out")
            IoError(msg) => {
                var s = string("ProcessError: I/O error: ")
                s.append_view(msg.to_view())
                return s
            }
        }
    }
}

public struct UnitTy {}

/// How a child's standard stream is connected.
public variant Stdio {
    Inherit()
    Piped()
    Null()
}

/// Exit status of a finished child.
public struct ExitStatus {
    internal var _code : int;
    internal var _signaled : bool;
    internal var _signal : int;

    /// Exit code, or `None` when the child was terminated by a signal.
    public func code(&self) : Option<int> {
        if(self._signaled) { return Option.None<int>() }
        return Option.Some<int>(self._code)
    }

    /// True when the child exited normally with code 0.
    public func success(&self) : bool {
        return !self._signaled && self._code == 0
    }

    /// True when a signal terminated the child.
    public func signaled(&self) : bool {
        return self._signaled
    }

    /// Terminating signal number, when applicable.
    public func signal(&self) : Option<int> {
        if(!self._signaled) { return Option.None<int>() }
        return Option.Some<int>(self._signal)
    }
}

/// Captured stdout/stderr and the final exit status.
public struct Output {
    internal var _status : ExitStatus
    internal var stdout_data : vector<u8>
    internal var stderr_data : vector<u8>

    public func status(&self) : ExitStatus {
        return self._status
    }

    public func success(&self) : bool {
        return self._status.success()
    }

    public func is_success(&self) : bool {
        return self._status.success()
    }

    public func exit_code(&self) : int {
        var c = self._status.code()
        if(c is Option.None) { return -1 }
        var Some(v) = c else unsafe { unreachable }
        return v
    }

    public func stdout_size(&self) : size_t {
        return self.stdout_data.size()
    }

    public func stderr_size(&self) : size_t {
        return self.stderr_data.size()
    }

    public func stdout_bytes(&self) : [u8] {
        return unsafe(std::span<u8>(self.stdout_data.data(), self.stdout_data.size()))
    }

    public func stderr_bytes(&self) : [u8] {
        return unsafe(std::span<u8>(self.stderr_data.data(), self.stderr_data.size()))
    }

    /// Stdout as UTF-8, or `None` when the bytes are not valid UTF-8.
    public func stdout_str(&self) : Option<std::str> {
        var view = unsafe(string_view(self.stdout_data.data() as *char, self.stdout_data.size()))
        var made = std::try_from_utf8(view)
        if(made is Result.Err) { return Option.None<std::str>() }
        var Ok(text) = made else unsafe { unreachable }
        return Option.Some<std::str>(text)
    }

    /// Stderr as UTF-8, or `None` when the bytes are not valid UTF-8.
    public func stderr_str(&self) : Option<std::str> {
        var view = unsafe(string_view(self.stderr_data.data() as *char, self.stderr_data.size()))
        var made = std::try_from_utf8(view)
        if(made is Result.Err) { return Option.None<std::str>() }
        var Ok(text) = made else unsafe { unreachable }
        return Option.Some<std::str>(text)
    }
}

} // end namespace process

public namespace process {

// Conversion into the shared I/O error used by Read/Write/network APIs.
public func io_error(e : ProcessError) : std::io::IoError {
    switch(e) {
        InvalidArgs(msg) => { return std::io::IoError.Message(std::io::IoErrorKind.InvalidInput(), msg) }
        OperationFailed(msg) => { return std::io::IoError.Message(std::io::IoErrorKind.Other(), msg) }
        NotRunning() => { return std::io::IoError.Kind(std::io::IoErrorKind.Other()) }
        TimedOut() => { return std::io::IoError.Kind(std::io::IoErrorKind.WouldBlock()) }
        IoError(msg) => { return std::io::IoError.Message(std::io::IoErrorKind.Other(), msg) }
    }
}

}
