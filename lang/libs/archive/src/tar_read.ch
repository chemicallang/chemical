public namespace archive {

using std::Result;
using std::string;
using std::vector;

@direct_init
public struct TarArchive {
    var data : vector<u8>
    var entries : vector<ArchiveEntry>
    var data_loaded : bool
}

public func open_tar(path : *char, output : *mut TarArchive) : std::Result<std::Unit, ArchiveError> {
    var read_result = fs::read_entire_file(path)
    if(read_result is Result.Err) {
        return std.Result.Err(ArchiveError.FileNotFound())
    }
    var Ok(fd) = read_result else unreachable
    memcpy(&raw mut output.data, &raw fd, sizeof(vector<u8>))
    new(&raw fd) vector<u8>()

    output.data_loaded = true

    var parse_result = parse_tar_entries(output)
    if(parse_result is Result.Err) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("failed to parse TAR entries")))
    }

    return std.Result.Ok(std::Unit{})
}

func parse_tar_entries(archive : *mut TarArchive) : std::Result<std::Unit, ArchiveError> {
    var pos : size_t = 0
    var data_size = archive.data.size()

    while(pos + 512 <= data_size) {
        var all_zero = true
        var i : size_t = 0
        while(i < 512) {
            if(archive.data.data()[pos + i] != 0) {
                all_zero = false
                break
            }
            i += 1
        }
        if(all_zero) { break }

        var name = string("")
        var j : size_t = 0
        while(j < 100 && archive.data.data()[pos + j] != 0) {
            name.append(archive.data.data()[pos + j] as char)
            j += 1
        }

        var size_str = string("")
        j = 124
        while(j < 136 && archive.data.data()[pos + j] != 0) {
            size_str.append(archive.data.data()[pos + j] as char)
            j += 1
        }
        var size = tar_parse_octal(&raw mut size_str)

        var typeflag = archive.data.data()[pos + 156]

        var prefix = string("")
        j = 345
        while(j < 500 && archive.data.data()[pos + j] != 0) {
            prefix.append(archive.data.data()[pos + j] as char)
            j += 1
        }

        var full_name = string("")
        if(prefix.size() > 0) {
            full_name.append_string(&prefix)
            full_name.append('/')
        }
        full_name.append_string(&name)

        var is_dir = (typeflag == '5' as u8) || (typeflag == '/' as u8)
        if(name.size() > 0 && name.get(name.size() - 1) == '/' as char) {
            is_dir = true
        }

        var entry : ArchiveEntry
        entry.name = full_name
        entry.size = size
        entry.compressed_size = size
        entry.compression_method = 0
        entry.crc32 = 0
        entry.is_directory = is_dir
        entry.offset = (pos + 512) as u64

        archive.entries.push(entry)

        var data_blocks = (size + 511) / 512
        pos += 512 + (data_blocks * 512)
    }

    return std.Result.Ok(std::Unit{})
}

func tar_parse_octal(str : *mut string) : u64 {
    var result : u64 = 0
    var i : size_t = 0
    while(i < str.size()) {
        var c = str.get(i)
        if(c >= '0' as char && c <= '7' as char) {
            result = result * 8 + ((c as u64) - ('0' as u64))
        }
        i += 1
    }
    return result
}

public func tar_entries(archive : *mut TarArchive) : *mut vector<ArchiveEntry> {
    return &raw mut archive.entries
}

public func tar_entry_count(archive : *mut TarArchive) : size_t {
    return archive.entries.size()
}

public func tar_find_entry(archive : *mut TarArchive, name : *char, output : *mut ArchiveEntry) : std::Result<std::Unit, ArchiveError> {
    var i : size_t = 0
    while(i < archive.entries.size()) {
        var entry = archive.entries.get_ptr(i)
        if(str_equals_cstr(&raw mut entry.name, name)) {
            memcpy(&raw mut output, &raw entry, sizeof(ArchiveEntry))
            new(entry) ArchiveEntry{name: string(""), size: 0, compressed_size: 0, compression_method: 0, crc32: 0, is_directory: false, offset: 0}
            return std.Result.Ok(std::Unit{})
        }
        i += 1
    }
    return std.Result.Err(ArchiveError.FileNotFound())
}

public func tar_read_entry(archive : *mut TarArchive, entry : *mut ArchiveEntry, output : *mut vector<u8>) : std::Result<std::Unit, ArchiveError> {
    if(!archive.data_loaded) {
        return std.Result.Err(ArchiveError.IoError(string("archive data not loaded")))
    }

    var offset = entry.offset as size_t
    var size = entry.size as size_t

    if(offset + size > archive.data.size()) {
        return std.Result.Err(ArchiveError.InvalidFormat(string("entry data extends beyond file")))
    }

    output.resize(size)
    var ptr = output.data() as *mut u8
    var i : size_t = 0
    while(i < size) {
        ptr[i] = archive.data.data()[offset + i]
        i += 1
    }

    return std.Result.Ok(std::Unit{})
}

public func tar_read_file(archive : *mut TarArchive, name : *char) : std::Result<vector<u8>, ArchiveError> {
    var entry : ArchiveEntry
    var entry_result = tar_find_entry(archive, name, &raw mut entry)
    if(entry_result is Result.Err) {
        return std.Result.Err(ArchiveError.FileNotFound())
    }
    var content = vector<u8>()
    var content_result = tar_read_entry(archive, &raw mut entry, &raw mut content)
    if(content_result is Result.Err) {
        return std.Result.Err(ArchiveError.IoError(string("failed to read entry data")))
    }
    return std.Result.Ok(content)
}

public func tar_contains(archive : *mut TarArchive, name : *char) : bool {
    var entry : ArchiveEntry
    var r = tar_find_entry(archive, name, &raw mut entry)
    return r is Result.Ok
}

public func tar_extract_entry(archive : *mut TarArchive, entry : *mut ArchiveEntry, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
    var full_path = string(dest_dir)
    if(full_path.size() > 0 && full_path.get(full_path.size() - 1) != '/' as char) {
        full_path.append('/')
    }
    full_path.append_string(&entry.name)

    if(entry.is_directory) {
        fs::create_dir_all(full_path.c_str())
        return std.Result.Ok(std::Unit{})
    }

    var content = vector<u8>()
    var content_result = tar_read_entry(archive, entry, &raw mut content)
    if(content_result is Result.Err) {
        return std.Result.Err(ArchiveError.IoError(string("failed to read entry for extraction")))
    }

    fs::create_dir_all(full_path.c_str())

    var write_result = fs::write_text_file(full_path.c_str(), content.data(), content.size())
    if(write_result is Result.Err) {
        return std.Result.Err(ArchiveError.IoError(string("failed to write file")))
    }

    return std.Result.Ok(std::Unit{})
}

public func tar_extract_all(archive : *mut TarArchive, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
    fs::create_dir_all(dest_dir)
    var i : size_t = 0
    while(i < archive.entries.size()) {
        var entry = archive.entries.get_ptr(i)
        var result = tar_extract_entry(archive, entry, dest_dir)
        if(result is Result.Err) {
            return std.Result.Err(ArchiveError.IoError(string("failed to extract entry")))
        }
        i += 1
    }
    return std.Result.Ok(std::Unit{})
}

} // end namespace archive
