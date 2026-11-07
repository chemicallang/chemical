public namespace environment {

using std::string;
using std::string_view;

public variant EnvError {
    OperationFailed(msg : string)

    func message(&self) : string {
        switch(self) {
            OperationFailed(msg) => {
                var s = string("EnvError: operation failed: ")
                const msg_view = unsafe(string_view(msg.data(), msg.size()))
                s.append_view(&msg_view)
                return s
            }
        }
    }
}

} // end namespace environment

public namespace environment {

public func io_error(e : EnvError) : std::io::IoError {
    switch(e) {
        OperationFailed(msg) => { return std::io::IoError.Message(std::io::IoErrorKind.Other(), msg) }
    }
}

}
