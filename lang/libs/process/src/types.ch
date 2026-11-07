public namespace process {

using std::string;
using std::string_view;

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
