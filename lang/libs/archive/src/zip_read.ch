public namespace archive {

using std::Result;
using std::string;
using std::vector;

using fs::File;
using fs::OpenOptions;

const ZIP_LOCAL_FILE_HEADER : u32 = 0x04034B50u32
const ZIP_CENTRAL_DIR_HEADER : u32 = 0x02014B50u32
const ZIP_END_OF_CENTRAL_DIR : u32 = 0x06054B50u32

@direct_init
public struct ZipArchive {
    var data : vector<u8>
    var entries : vector<ArchiveEntry>
    var data_loaded : bool
}

public func open_zip(path : *char, output : *mut ZipArchive) : std::Result<std::Unit, ArchiveError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(ArchiveError.FileNotFound())
    }
    var Ok(fd) = read_result else unreachable
    memcpy(&raw mut output.data, &raw fd, sizeof(vector<u8>))
    new(&raw fd) vector<u8>()

    output.data_loaded = true

    var parse_result = parse_zip_central_dir(output)
    if(parse_result is Result.Err) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("failed to parse ZIP central directory")))
    }

    return std.Result.Ok(std::Unit{})
}

public func open_zip_bytes(data : *u8, data_len : size_t, output : *mut ZipArchive) : std::Result<std::Unit, ArchiveError> {
    var vec = vector<u8>()
    vec.resize(data_len)
    var i : size_t = 0
    var vptr = vec.data() as *mut u8
    while(i < data_len) {
        vptr[i] = data[i]
        i += 1
    }
    output.data = vec
    output.data_loaded = true

    var parse_result = parse_zip_central_dir(output)
    if(parse_result is Result.Err) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("failed to parse ZIP central directory")))
    }

    return std.Result.Ok(std::Unit{})
}

func parse_zip_central_dir(archive : *mut ZipArchive) : std::Result<std::Unit, ArchiveError> {
    if(archive.data.size() < 22) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("file too small for ZIP")))
    }

    var eocd_pos : size_t = 0
    var found = false
    var search_pos = archive.data.size() - 22
    while(search_pos > 0) {
        var sig = read_u32_le(archive.data.data(), search_pos)
        if(sig == ZIP_END_OF_CENTRAL_DIR) {
            eocd_pos = search_pos
            found = true
            break
        }
        search_pos -= 1
    }

    if(!found) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("no End of Central Directory found")))
    }

    var num_entries = read_u16_le(archive.data.data(), eocd_pos + 10) as size_t
    var cd_offset = read_u32_le(archive.data.data(), eocd_pos + 16) as size_t

    var pos = cd_offset
    var i : size_t = 0
    while(i < num_entries) {
        if(pos + 46 > archive.data.size()) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("truncated central directory entry")))
        }

        var sig = read_u32_le(archive.data.data(), pos)
        if(sig != ZIP_CENTRAL_DIR_HEADER) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("invalid central directory entry signature")))
        }

        var method = read_u16_le(archive.data.data(), pos + 10)
        var crc = read_u32_le(archive.data.data(), pos + 16)
        var comp_size = read_u32_le(archive.data.data(), pos + 20) as u64
        var uncomp_size = read_u32_le(archive.data.data(), pos + 24) as u64
        var name_len = read_u16_le(archive.data.data(), pos + 28) as size_t
        var extra_len = read_u16_le(archive.data.data(), pos + 30) as size_t
        var comment_len = read_u16_le(archive.data.data(), pos + 32) as size_t
        var local_offset = read_u32_le(archive.data.data(), pos + 42) as u64

        var name_start = pos + 46
        var name = string("")
        var j : size_t = 0
        while(j < name_len && name_start + j < archive.data.size()) {
            name.append(archive.data.data()[name_start + j] as char)
            j += 1
        }

        var is_dir = false
        if(name_len > 0) {
            var last_char = archive.data.data()[name_start + name_len - 1]
            if(last_char == '/' as u8 || last_char == '\\' as u8) {
                is_dir = true
            }
        }

        var entry : ArchiveEntry
        entry.name = name
        entry.size = uncomp_size
        entry.compressed_size = comp_size
        entry.compression_method = method
        entry.crc32 = crc
        entry.is_directory = is_dir
        entry.offset = local_offset

        archive.entries.push(entry)

        pos += 46 + name_len + extra_len + comment_len
        i += 1
    }

    return std.Result.Ok(std::Unit{})
}

public func zip_entries(archive : *mut ZipArchive) : *mut vector<ArchiveEntry> {
    return &raw mut archive.entries
}

public func zip_entry_count(archive : *mut ZipArchive) : size_t {
    return archive.entries.size()
}

func str_equals_cstr(s : *mut string, cstr : *char) : bool {
    var i : size_t = 0
    var s_len = s.size()
    while(i < s_len) {
        if(cstr[i] == '\0' as char) { return false }
        if(s.get(i) != cstr[i]) { return false }
        i += 1
    }
    return cstr[i] == '\0' as char
}

public func zip_find_entry(archive : *mut ZipArchive, name : *char, output : *mut ArchiveEntry) : std::Result<std::Unit, ArchiveError> {
    var i : size_t = 0
    while(i < archive.entries.size()) {
        var entry = archive.entries.get_ptr(i)
        if(str_equals_cstr(&raw mut entry.name, name)) {
            memcpy(output, &raw entry, sizeof(ArchiveEntry))
            new(entry) ArchiveEntry{name: string(""), size: 0, compressed_size: 0, compression_method: 0, crc32: 0, is_directory: false, offset: 0}
            return std.Result.Ok(std::Unit{})
        }
        i += 1
    }
    return std.Result.Err(ArchiveError.FileNotFound())
}

public func zip_read_entry(archive : *mut ZipArchive, entry : *mut ArchiveEntry, output : *mut vector<u8>) : std::Result<std::Unit, ArchiveError> {
    if(!archive.data_loaded) {
        return std.Result.Err(ArchiveError.IoError(string("archive data not loaded")))
    }

    var pos = entry.offset as size_t
    if(pos + 30 > archive.data.size()) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("truncated local file header")))
    }

    var sig = read_u32_le(archive.data.data(), pos)
    if(sig != ZIP_LOCAL_FILE_HEADER) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("invalid local file header signature")))
    }

    var local_name_len = read_u16_le(archive.data.data(), pos + 26) as size_t
    var local_extra_len = read_u16_le(archive.data.data(), pos + 28) as size_t
    var data_offset = pos + 30 + local_name_len + local_extra_len

    if(entry.compression_method == 0) {
        var size = entry.compressed_size as size_t
        output.resize(size)
        var ptr = output.data() as *mut u8
        var i : size_t = 0
        while(i < size && data_offset + i < archive.data.size()) {
            ptr[i] = archive.data.data()[data_offset + i]
            i += 1
        }
        return std.Result.Ok(std::Unit{})
    } else if(entry.compression_method == 8) {
        if(data_offset + (entry.compressed_size as size_t) > archive.data.size()) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("compressed data extends beyond file")))
        }
        var comp_data = archive.data.data() + data_offset
        var comp_len = entry.compressed_size as size_t
        var uncomp_len = entry.size as size_t

        output.resize(uncomp_len)

        var def_result = deflate_decompress(comp_data, comp_len, output.data() as *mut u8, uncomp_len)
        if(def_result is Result.Err) {
            return std.Result.Err(ArchiveError.DecompressionFailed(string("deflate decompression failed")))
        }

        var Ok(bytes_written) = def_result else unreachable
        var computed_crc = crc32_compute(output.data(), bytes_written)
        if(computed_crc != entry.crc32) {
            return std.Result.Err(ArchiveError.CrcMismatch())
        }

        return std.Result.Ok(std::Unit{})
    } else {
        return std.Result.Err(ArchiveError.UnsupportedCompression(entry.compression_method as int))
    }
}

public func zip_read_file(archive : *mut ZipArchive, name : *char) : std::Result<vector<u8>, ArchiveError> {
    var entry : ArchiveEntry
    var entry_result = zip_find_entry(archive, name, &raw mut entry)
    if(entry_result is Result.Err) {
        return std.Result.Err(ArchiveError.FileNotFound())
    }
    var content = vector<u8>()
    var content_result = zip_read_entry(archive, &raw mut entry, &raw mut content)
    if(content_result is Result.Err) {
        return std.Result.Err(ArchiveError.DecompressionFailed(string("failed to read entry")))
    }
    return std.Result.Ok(content)
}

public func zip_contains(archive : *mut ZipArchive, name : *char) : bool {
    var entry : ArchiveEntry
    var r = zip_find_entry(archive, name, &raw mut entry)
    return r is Result.Ok
}

public func zip_extract_entry(archive : *mut ZipArchive, entry : *mut ArchiveEntry, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
    var content = vector<u8>()
    var content_result = zip_read_entry(archive, entry, &raw mut content)
    if(content_result is Result.Err) {
        return std.Result.Err(ArchiveError.DecompressionFailed(string("failed to read entry for extraction")))
    }

    var full_path = string(dest_dir)
    if(full_path.size() > 0) {
        var last = full_path.get(full_path.size() - 1)
        if(last != '/' as char) {
            full_path.append('/')
        }
    }
    full_path.append_string(&entry.name)

    if(!entry.is_directory) {
        var parent = fs::parent_path_view(std::string_view(full_path.data(), full_path.size()))
        fs::create_dir_all(parent.c_str())
    }

    if(entry.is_directory) {
        fs::create_dir_all(full_path.c_str())
        return std.Result.Ok(std::Unit{})
    }

    var write_result = fs::write_text_file(full_path.c_str(), content.data(), content.size())
    if(write_result is Result.Err) {
        return std.Result.Err(ArchiveError.IoError(string("failed to write file")))
    }

    return std.Result.Ok(std::Unit{})
}

public func zip_extract_all(archive : *mut ZipArchive, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
    fs::create_dir_all(dest_dir)
    var i : size_t = 0
    while(i < archive.entries.size()) {
        var entry = archive.entries.get_ptr(i)
        var result = zip_extract_entry(archive, entry, dest_dir)
        if(result is Result.Err) {
            return std.Result.Err(ArchiveError.IoError(string("failed to extract entry")))
        }
        i += 1
    }
    return std.Result.Ok(std::Unit{})
}

} // end namespace archive
