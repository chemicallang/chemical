public namespace crypto {

using std::string;

public variant CryptoError {
    BufferTooSmall(needed : size_t)
    InvalidInput()
    UnknownAlgorithm()

    func message(&self) : string {
        switch(self) {
            BufferTooSmall(needed) => {
                var s = string("CryptoError: buffer too small, need ")
                s.append_integer(needed as int)
                return s
            }
            InvalidInput() => return string("CryptoError: invalid input")
            UnknownAlgorithm() => return string("CryptoError: unknown algorithm")
        }
    }
}

} // end namespace crypto
