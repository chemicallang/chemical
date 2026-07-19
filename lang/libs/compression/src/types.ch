public namespace compression {

using std::string;

public variant CompressionError {
    BufferTooSmall(needed : size_t)
    InvalidData()
    UnsupportedAlgorithm()

    func message(&self) : string {
        switch(self) {
            BufferTooSmall(needed) => {
                var s = string("CompressionError: buffer too small, need ")
                s.append_integer(needed as int)
                return s
            }
            InvalidData() => return string("CompressionError: invalid compressed data")
            UnsupportedAlgorithm() => return string("CompressionError: unsupported compression algorithm")
        }
    }
}

} // end namespace compression
