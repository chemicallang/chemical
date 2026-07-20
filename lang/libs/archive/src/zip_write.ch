public namespace archive {

using std::Result;
using std::string;
using std::vector;

using fs::File;
using fs::OpenOptions;

@direct_init
public struct ZipWriter {
    var entries : vector<ZipWriteEntry>
    var data : vector<u8>
}

public struct ZipWriteEntry {
    var name : string
    var uncompressed_size : u32
    var compressed_size : u32
    var crc : u32
    var method : u16
    var local_header_offset : u32
}

public func zip_writer_add_file(writer : *mut ZipWriter, name : *char, file_data : *u8, file_len : size_t) {
    var crc = crc32_compute(file_data, file_len)

    var entry : ZipWriteEntry
    entry.name = string("")
    entry.name.append_char_ptr(name)
    entry.uncompressed_size = file_len as u32
    entry.compressed_size = file_len as u32
    entry.crc = crc
    entry.method = 0 // STORE
    entry.local_header_offset = writer.data.size() as u32

    write_local_header(writer, name, file_len as u32, crc)

    var i : size_t = 0
    while(i < file_len) {
        writer.data.push(file_data[i])
        i += 1
    }

    writer.entries.push(entry)
}

public func zip_writer_add_bytes(writer : *mut ZipWriter, name : *char, data : *u8, data_len : size_t) {
    zip_writer_add_file(writer, name, data, data_len)
}

public func zip_writer_add_store(writer : *mut ZipWriter, name : *char, data : *u8, data_len : size_t) {
    zip_writer_add_file(writer, name, data, data_len)
}

func write_local_header(writer : *mut ZipWriter, name : *char, size : u32, crc : u32) {
    var name_len : size_t = 0
    var temp = name
    while(temp[0] != '\0' as char) {
        name_len += 1
        temp = temp + 1
    }

    var header_size : size_t = 30 + name_len
    var header = vector<u8>()
    header.resize(header_size)

    write_u32_le(header.data() as *mut u8, 0, ZIP_LOCAL_FILE_HEADER)
    write_u16_le(header.data() as *mut u8, 4, 20) // version needed
    write_u16_le(header.data() as *mut u8, 6, 0)  // flags
    write_u16_le(header.data() as *mut u8, 8, 0)  // STORE
    write_u16_le(header.data() as *mut u8, 10, 0) // mod time
    write_u16_le(header.data() as *mut u8, 12, 0) // mod date
    write_u32_le(header.data() as *mut u8, 14, crc)
    write_u32_le(header.data() as *mut u8, 18, size) // compressed size
    write_u32_le(header.data() as *mut u8, 22, size) // uncompressed size
    write_u16_le(header.data() as *mut u8, 26, name_len as u16)
    write_u16_le(header.data() as *mut u8, 28, 0) // extra field length

    var i : size_t = 0
    var hptr = header.data() as *mut u8
    while(i < name_len) {
        hptr[30 + i] = name[i] as u8
        i += 1
    }

    i = 0
    while(i < header_size) {
        writer.data.push(header.data()[i])
        i += 1
    }
}

public func zip_writer_close(writer : *mut ZipWriter) {
    var cd_offset = writer.data.size() as u32
    var cd_size : u32 = 0

    var i : size_t = 0
    while(i < writer.entries.size()) {
        var entry = writer.entries.get_ptr(i)
        var name_len = entry.name.size() as u16

        var header = vector<u8>()
        header.resize(46 + name_len as size_t)

        write_u32_le(header.data() as *mut u8, 0, ZIP_CENTRAL_DIR_HEADER)
        write_u16_le(header.data() as *mut u8, 4, 20) // version made by
        write_u16_le(header.data() as *mut u8, 6, 20) // version needed
        write_u16_le(header.data() as *mut u8, 8, 0)  // flags
        write_u16_le(header.data() as *mut u8, 10, entry.method)
        write_u16_le(header.data() as *mut u8, 12, 0) // mod time
        write_u16_le(header.data() as *mut u8, 14, 0) // mod date
        write_u32_le(header.data() as *mut u8, 16, entry.crc)
        write_u32_le(header.data() as *mut u8, 20, entry.compressed_size)
        write_u32_le(header.data() as *mut u8, 24, entry.uncompressed_size)
        write_u16_le(header.data() as *mut u8, 28, name_len)
        write_u16_le(header.data() as *mut u8, 30, 0) // extra field length
        write_u16_le(header.data() as *mut u8, 32, 0) // comment length
        write_u16_le(header.data() as *mut u8, 34, 0) // disk number start
        write_u16_le(header.data() as *mut u8, 36, 0) // internal attributes
        write_u32_le(header.data() as *mut u8, 38, 0) // external attributes
        write_u32_le(header.data() as *mut u8, 42, entry.local_header_offset)

        var j : u16 = 0
        var hptr2 = header.data() as *mut u8
        while(j < name_len) {
            hptr2[46 + j as size_t] = entry.name.get(j as size_t) as u8
            j += 1
        }

        var k : size_t = 0
        while(k < (46 + name_len as size_t)) {
            writer.data.push(header.data()[k])
            k += 1
        }

        cd_size += (46 + name_len as u32)
        i += 1
    }

    var eocd = vector<u8>()
    eocd.resize(22)

    write_u32_le(eocd.data() as *mut u8, 0, ZIP_END_OF_CENTRAL_DIR)
    write_u16_le(eocd.data() as *mut u8, 4, 0) // disk number
    write_u16_le(eocd.data() as *mut u8, 6, 0) // disk with central dir
    write_u16_le(eocd.data() as *mut u8, 8, writer.entries.size() as u16) // entries on this disk
    write_u16_le(eocd.data() as *mut u8, 10, writer.entries.size() as u16) // total entries
    write_u32_le(eocd.data() as *mut u8, 12, cd_size)
    write_u32_le(eocd.data() as *mut u8, 16, cd_offset)
    write_u16_le(eocd.data() as *mut u8, 20, 0) // comment length

    var m : size_t = 0
    while(m < 22) {
        writer.data.push(eocd.data()[m])
        m += 1
    }
}

public func zip_writer_to_bytes(writer : *mut ZipWriter) : *mut vector<u8> {
    zip_writer_close(writer)
    return &raw mut writer.data
}

public func zip_writer_save(writer : *mut ZipWriter, path : *char) : std::Result<std::Unit, ArchiveError> {
    zip_writer_close(writer)

    var write_result = fs::write_text_file(path, writer.data.data(), writer.data.size())
    if(write_result is Result.Err) {
        return std.Result.Err(ArchiveError.IoError(string("failed to write ZIP data")))
    }

    return std.Result.Ok(std::Unit{})
}

} // end namespace archive
