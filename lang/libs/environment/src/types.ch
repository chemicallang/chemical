public namespace environment {

using std::string;

public variant EnvError {
    OperationFailed(msg : string)

    func message(&self) : string {
        switch(self) {
            OperationFailed(msg) => {
                var s = string("EnvError: operation failed: ")
                s.append_view(string_view(msg.data(), msg.size()))
                return s
            }
        }
    }
}

} // end namespace environment
