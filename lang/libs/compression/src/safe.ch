// safe — owned-buffer compression surface.
//
// `compress`/`decompress` take `&[u8]` and return owned `vector<u8>` buffers.
// Allocation failure is reported through `CompressionError.OutOfMemory()`
// instead of aborting, and every malformed RLE stream is rejected.
//
// The legacy raw RLE/out-buffer entry points live under `compression::raw`.
//
// Wire format (matching the legacy RLE codec): a sequence of
// `(count : u8, byte : u8)` pairs, terminated by a single `0` byte. No count
// may be zero except the terminator, and no trailer is allowed after it.

public namespace compression {

using std::Result;
using std::vector;

/// Compress bytes with RLE. The output always includes the `0` terminator.
public func compress(data : &[u8]) : Result<vector<u8>, CompressionError> {
    const n = data.size()
    // Worst case is one (count, byte) pair per input byte plus the terminator.
    if(n > (((0 as size_t) - 1) - 2) / 2) {
        return Result.Err(CompressionError.InvalidData())
    }
    var out = vector<u8>()
    if(!out.try_reserve(n * 2 + 2)) {
        return Result.Err(CompressionError.OutOfMemory())
    }
    var ipos : size_t = 0
    while(ipos < n) {
        var current = unsafe(*data.at(ipos))
        var run_len : u8 = 1
        ipos += 1
        while(ipos < n && run_len < 255 && unsafe(*data.at(ipos)) == current) {
            run_len += 1
            ipos += 1
        }
        if(!out.try_push(run_len)) {
            return Result.Err(CompressionError.OutOfMemory())
        }
        if(!out.try_push(current)) {
            return Result.Err(CompressionError.OutOfMemory())
        }
    }
    if(!out.try_push(0)) {
        return Result.Err(CompressionError.OutOfMemory())
    }
    return Result.Ok(out)
}

/// Decompress an RLE stream. Requires the `0` terminator and rejects a
/// truncated pair, a missing terminator, or bytes after the terminator.
public func decompress(data : &[u8]) : Result<vector<u8>, CompressionError> {
    var out = vector<u8>()
    var ipos : size_t = 0
    while(ipos < data.size()) {
        var count = unsafe(*data.at(ipos))
        ipos += 1
        if(count == 0) {
            if(ipos != data.size()) {
                return Result.Err(CompressionError.InvalidData())
            }
            return Result.Ok(out)
        }
        if(ipos >= data.size()) {
            return Result.Err(CompressionError.InvalidData())
        }
        var byte_val = unsafe(*data.at(ipos))
        ipos += 1
        var add = count as size_t
        if(out.size() > ((0 as size_t) - 1) - add) {
            return Result.Err(CompressionError.InvalidData())
        }
        if(!out.try_reserve(out.size() + add)) {
            return Result.Err(CompressionError.OutOfMemory())
        }
        var i : u16 = 0
        while(i < add as u16) {
            if(!out.try_push(byte_val)) {
                return Result.Err(CompressionError.OutOfMemory())
            }
            i += 1
        }
    }
    // No terminator: the stream is truncated.
    return Result.Err(CompressionError.InvalidData())
}

} // end namespace compression
