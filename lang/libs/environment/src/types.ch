public namespace environment {

using std::string;
using std::string_view;

public variant EnvError {
    OperationFailed(msg : string)
    NotFound()
    InvalidInput()
    OutOfMemory()

    func message(&self) : string {
        switch(self) {
            OperationFailed(msg) => {
                var s = string("EnvError: operation failed: ")
                s.append_view(msg.to_view())
                return s
            }
            NotFound() => return string("EnvError: variable not found")
            InvalidInput() => return string("EnvError: invalid input")
            OutOfMemory() => return string("EnvError: out of memory")
        }
    }
}

} // end namespace environment

public namespace environment {

public func io_error(e : EnvError) : std::io::IoError {
    switch(e) {
        OperationFailed(msg) => { return std::io::IoError.Message(std::io::IoErrorKind.Other(), msg) }
        NotFound() => { return std::io::IoError.Kind(std::io::IoErrorKind.NotFound()) }
        InvalidInput() => { return std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()) }
        OutOfMemory() => { return std::io::IoError.Kind(std::io::IoErrorKind.OutOfMemory()) }
    }
}

}
