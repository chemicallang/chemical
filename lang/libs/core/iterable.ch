public namespace core {

public namespace iterable {

public interface Linear<T> {

    func data(&self) : *T

    func size(&self) : u64

}

public struct Chunk<T> {

    var ptr : *T
    var len : u64

    func data(&self) : *T {
        return ptr
    }

    func size(&self) : u64 {
        return len
    }

}

public interface Chunked<T, ChunkCursor> {

    func begin_chunks(&self) : ChunkCursor

    func valid_chunk(&self, c : ChunkCursor) : bool

    func current_chunk(&self, c : ChunkCursor) : Chunk<T>

    func next_chunk(&self, c : ChunkCursor) : ChunkCursor

    func rbegin_chunks(&self) : ChunkCursor

    func previous_chunk(&self, c : ChunkCursor) : ChunkCursor

    func total_size(&self) : u64

}

public interface Iterable<T, Cursor> {

    func begin(&self) : Cursor

    func valid(&self, c : Cursor) : bool

    func current(&self, c : Cursor) : &T

    func next(&self, c : Cursor) : Cursor

}

}

}
