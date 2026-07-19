public namespace path {

using std::string;
using std::string_view;

public variant PathError {
    BufferTooSmall()
    TooManyComponents()
    InvalidPath(msg : string)

    func message(&self) : string {
        switch(self) {
            BufferTooSmall() => return string("PathError: output buffer too small")
            TooManyComponents() => return string("PathError: too many path components")
            InvalidPath(msg) => {
                var s = string("PathError: invalid path: ")
                s.append_string(&msg)
                return s
            }
        }
    }
}

} // end namespace path
