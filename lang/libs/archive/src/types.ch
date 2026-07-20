public namespace archive {

using std::string;
using std::vector;

public variant ArchiveError {
    FileNotFound()
    InvalidFormat(msg : string)
    UnsupportedCompression(method : int)
    DecompressionFailed(msg : string)
    CrcMismatch()
    IoError(msg : string)
    BufferTooSmall()
    FileAlreadyExists(name : string)

    func message(&self) : string {
        switch(self) {
            FileNotFound() => return string("ArchiveError: file not found")
            InvalidFormat(msg) => {
                var s = string("ArchiveError: ")
                s.append_string(&msg)
                return s
            }
            UnsupportedCompression(method) => {
                var s = string("ArchiveError: unsupported compression method ")
                s.append_integer(method)
                return s
            }
            DecompressionFailed(msg) => {
                var s = string("ArchiveError: decompression failed: ")
                s.append_string(&msg)
                return s
            }
            CrcMismatch() => return string("ArchiveError: CRC mismatch")
            IoError(msg) => {
                var s = string("ArchiveError: IO error: ")
                s.append_string(&msg)
                return s
            }
            BufferTooSmall() => return string("ArchiveError: buffer too small")
            FileAlreadyExists(name) => {
                var s = string("ArchiveError: file already exists: ")
                s.append_string(&name)
                return s
            }
        }
    }
}

@direct_init
public struct ArchiveEntry {
    var name : string
    var size : u64
    var compressed_size : u64
    var compression_method : u16
    var crc32 : u32
    var is_directory : bool
    var offset : u64
}

public enum CompressionMethod : u16 {
    Store = 0
    Deflate = 8
}

} // end namespace archive
