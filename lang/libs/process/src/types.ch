public namespace process {

using std::string;

public variant ProcessError {
    InvalidArgs(msg : string)
    OperationFailed(msg : string)
    NotRunning()
    TimedOut()

    func message(&self) : string {
        switch(self) {
            InvalidArgs(msg) => {
                var s = string("ProcessError: invalid args: ")
                s.append_view(string(msg))
                return s
            }
            OperationFailed(msg) => {
                var s = string("ProcessError: operation failed: ")
                s.append_view(string(msg))
                return s
            }
            NotRunning() => return string("ProcessError: process is not running")
            TimedOut() => return string("ProcessError: operation timed out")
        }
    }
}

public struct UnitTy {}

} // end namespace process
