public namespace encoding {

using std::string;

public variant EncodingError {
    BufferTooSmall(needed : size_t)
    InvalidInput()
    Unsupported()

    func message(&self) : string {
        switch(self) {
            BufferTooSmall(needed) => {
                var s = string("EncodingError: buffer too small, need ")
                s.append_integer(needed as int)
                return s
            }
            InvalidInput() => return string("EncodingError: invalid input")
            Unsupported() => return string("EncodingError: unsupported operation")
        }
    }
}

} // end namespace encoding
