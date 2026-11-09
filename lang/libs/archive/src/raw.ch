// Raw, unchecked archive handles and byte-oriented entry points. All of the
// raw ZIP/TAR implementation lives in this single block so the compiler sees
// exactly one `archive::raw` namespace declaration. Safe code uses
// `archive::Archive` and `archive::ZipWriter` in `safe.ch`.
public namespace archive {

public namespace raw {

using std::Result;
using std::string;
using std::vector;

// ── from main.ch ─────────────────────────────────────────────
// Raw, unchecked archive handles and byte-oriented entry points. Safe code uses
// `archive::Archive` (open/from_bytes/entries/read_entry/read_file/extract) and
// `archive::ZipWriter` instead.



@direct_init
public struct Archive {
    var zip_data : ZipArchive
    var tar_data : TarArchive
    var archive_type : int
    var loaded : bool
}

public func open(path : *char) : std::Result<Archive, ArchiveError> {
    // Initialize with proper values at declaration so no destructor is called
    // on uninitialized garbage (which would happen with field assignments).
    var archive = Archive{
        zip_data: ZipArchive{data: vector<u8>(), entries: vector<ArchiveEntry>(), data_loaded: false},
        tar_data: TarArchive{data: vector<u8>(), entries: vector<ArchiveEntry>(), data_loaded: false},
        archive_type: 0,
        loaded: false
    }
    var zip_result = open_zip(path, &raw mut archive.zip_data)
    if(zip_result is Result.Ok) {
        archive.archive_type = 1
        archive.loaded = true
        return std.Result.Ok(archive)
    }
    var tar_result = open_tar(path, &raw mut archive.tar_data)
    if(tar_result is Result.Ok) {
        archive.archive_type = 2
        archive.loaded = true
        return std.Result.Ok(archive)
    }
    return std.Result.Err(ArchiveError.InvalidFormat(string("not a recognized archive format")))
}

public func entries(archive : *mut Archive) : *mut vector<ArchiveEntry> {
        unsafe {
        if(archive.archive_type == 1) {
            return zip_entries(&raw mut archive.zip_data)
        } else {
            return tar_entries(&raw mut archive.tar_data)
        }

        }
    }

public func entry_count(archive : *mut Archive) : size_t {
        unsafe {
        if(archive.archive_type == 1) {
            return zip_entry_count(&raw mut archive.zip_data)
        } else {
            return tar_entry_count(&raw mut archive.tar_data)
        }

        }
    }

public func contains(archive : *mut Archive, name : *char) : bool {
        unsafe {
        if(archive.archive_type == 1) {
            return zip_contains(&raw mut archive.zip_data, name)
        } else {
            return tar_contains(&raw mut archive.tar_data, name)
        }

        }
    }

public func read(archive : *mut Archive, name : *char) : std::Result<vector<u8>, ArchiveError> {
        unsafe {
        if(archive.archive_type == 1) {
            return zip_read_file(&raw mut archive.zip_data, name)
        } else {
            return tar_read_file(&raw mut archive.tar_data, name)
        }

        }
    }

public func extract(archive : *mut Archive, dest : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        if(archive.archive_type == 1) {
            return zip_extract_all(&raw mut archive.zip_data, dest)
        } else {
            return tar_extract_all(&raw mut archive.tar_data, dest)
        }

        }
    }

public func extract_entry(archive : *mut Archive, name : *char, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var entry : ArchiveEntry
        if(archive.archive_type == 1) {
            var entry_result = zip_find_entry(&raw mut archive.zip_data, name, unsafe(&raw mut entry))
            if(entry_result is Result.Err) {
                return std.Result.Err(ArchiveError.InvalidFormat(string("entry not found")))
            }
            return zip_extract_entry(&raw mut archive.zip_data, unsafe(&raw mut entry), dest_dir)
        } else {
            var entry_result = tar_find_entry(&raw mut archive.tar_data, name, unsafe(&raw mut entry))
            if(entry_result is Result.Err) {
                return std.Result.Err(ArchiveError.InvalidFormat(string("entry not found")))
            }
            return tar_extract_entry(&raw mut archive.tar_data, unsafe(&raw mut entry), dest_dir)
        }

        }
    }

// ── from zip_read.ch ─────────────────────────────────────────────

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
        unsafe {
        var read_result = fs::raw::read_entire_file(path)
        if(read_result is Result.Err) {
            return std.Result.Err(ArchiveError.FileNotFound())
        }
        var Ok(fd) = read_result else unsafe { unreachable }
        unsafe { memcpy(&raw mut output.data, &raw fd, sizeof(vector<u8>)) }
        new(&raw fd) vector<u8>()

        output.data_loaded = true

        var parse_result = parse_zip_central_dir(output)
        if(parse_result is Result.Err) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("failed to parse ZIP central directory")))
        }

        return std.Result.Ok(std::Unit{})

        }
    }

public func open_zip_bytes(data : *u8, data_len : size_t, output : *mut ZipArchive) : std::Result<std::Unit, ArchiveError> {
        unsafe {
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
    }

func parse_zip_central_dir(archive : *mut ZipArchive) : std::Result<std::Unit, ArchiveError> {
        unsafe {
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

            var entry : ArchiveEntry = ArchiveEntry {
                name: name,
                size: uncomp_size,
                compressed_size: comp_size,
                compression_method: method,
                crc32: crc,
                is_directory: is_dir,
                offset: local_offset
            }

            archive.entries.push(entry)

            pos += 46 + name_len + extra_len + comment_len
            i += 1
        }

        return std.Result.Ok(std::Unit{})

        }
    }

public func zip_entries(archive : *mut ZipArchive) : *mut vector<ArchiveEntry> {
        unsafe {
        return &raw mut archive.entries

        }
    }

public func zip_entry_count(archive : *mut ZipArchive) : size_t {
        unsafe {
        return archive.entries.size()

        }
    }

func str_equals_cstr(s : *mut string, cstr : *char) : bool {
        unsafe {
        var i : size_t = 0
        var s_len = s.size()
        while(i < s_len) {
            if(cstr[i] == '\0' as char) { return false }
            if(s.at(i) != cstr[i]) { return false }
            i += 1
        }
        return cstr[i] == '\0' as char

        }
    }

public func zip_find_entry(archive : *mut ZipArchive, name : *char, output : *mut ArchiveEntry) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var i : size_t = 0
        while(i < archive.entries.size()) {
            var entry = archive.entries.get_ptr(i)
            if(str_equals_cstr(&raw mut entry.name, name)) {
                output.name = entry.name.copy()
                output.size = entry.size
                output.compressed_size = entry.compressed_size
                output.compression_method = entry.compression_method
                output.crc32 = entry.crc32
                output.is_directory = entry.is_directory
                output.offset = entry.offset
                return std.Result.Ok(std::Unit{})
            }
            i += 1
        }
        return std.Result.Err(ArchiveError.FileNotFound())

        }
    }

public func zip_read_entry(archive : *mut ZipArchive, entry : *mut ArchiveEntry, output : *mut vector<u8>) : std::Result<std::Unit, ArchiveError> {
        unsafe {
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

            var Ok(bytes_written) = def_result else unsafe { unreachable }
            var computed_crc = crc32_compute(output.data(), bytes_written)
            if(computed_crc != entry.crc32) {
                return std.Result.Err(ArchiveError.CrcMismatch())
            }

            return std.Result.Ok(std::Unit{})
        } else {
            return std.Result.Err(ArchiveError.UnsupportedCompression(entry.compression_method as int))
        }

        }
    }

public func zip_read_file(archive : *mut ZipArchive, name : *char) : std::Result<vector<u8>, ArchiveError> {
    var entry : ArchiveEntry
    var entry_result = zip_find_entry(archive, name, unsafe(&raw mut entry))
    if(entry_result is Result.Err) {
        return std.Result.Err(ArchiveError.FileNotFound())
    }
    var content = vector<u8>()
    var content_result = zip_read_entry(archive, unsafe(&raw mut entry), &raw mut content)
    if(content_result is Result.Err) {
        return std.Result.Err(ArchiveError.DecompressionFailed(string("failed to read entry")))
    }
    return std.Result.Ok(content)
}

public func zip_contains(archive : *mut ZipArchive, name : *char) : bool {
    var entry = ArchiveEntry {
        name : string(),
        size : 0u,
        compressed_size : 0u,
        compression_method : 0u,
        crc32 : 0u,
        is_directory : false,
        offset : 0u
    }
    var r = zip_find_entry(archive, name, unsafe(&raw mut entry))
    return r is Result.Ok
}

public func zip_extract_entry(archive : *mut ZipArchive, entry : *mut ArchiveEntry, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var content = vector<u8>()
        var content_result = zip_read_entry(archive, entry, &raw mut content)
        if(content_result is Result.Err) {
            return std.Result.Err(ArchiveError.DecompressionFailed(string("failed to read entry for extraction")))
        }

        var full_path = unsafe(string(dest_dir))
        if(full_path.size() > 0) {
            var last = full_path.at(full_path.size() - 1)
            if(last != '/' as char) {
                full_path.append('/')
            }
        }
        full_path.append_string(&entry.name)

        if(!entry.is_directory) {
            var parent = fs::raw::parent_path_view(unsafe(std::string_view(full_path.data(), full_path.size())))
            fs::raw::create_dir_all(parent.c_str())
        }

        if(entry.is_directory) {
            fs::raw::create_dir_all(full_path.c_str())
            return std.Result.Ok(std::Unit{})
        }

        var write_result = fs::raw::write_text_file(full_path.c_str(), content.data(), content.size())
        if(write_result is Result.Err) {
            return std.Result.Err(ArchiveError.IoError(string("failed to write file")))
        }

        return std.Result.Ok(std::Unit{})

        }
    }

public func zip_extract_all(archive : *mut ZipArchive, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        fs::raw::create_dir_all(dest_dir)
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
    }

// ── from tar_read.ch ─────────────────────────────────────────────

@direct_init
public struct TarArchive {
    var data : vector<u8>
    var entries : vector<ArchiveEntry>
    var data_loaded : bool
}

public func open_tar(path : *char, output : *mut TarArchive) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var read_result = fs::raw::read_entire_file(path)
        if(read_result is Result.Err) {
            return std.Result.Err(ArchiveError.FileNotFound())
        }
        var Ok(fd) = read_result else unsafe { unreachable }
        unsafe { memcpy(&raw mut output.data, &raw fd, sizeof(vector<u8>)) }
        new(&raw fd) vector<u8>()

        output.data_loaded = true

        if(output.data.size() < 512) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("file too small for TAR")))
        }

        var parse_result = parse_tar_entries(output)
        if(parse_result is Result.Err) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("failed to parse TAR entries")))
        }

        if(output.entries.size() == 0) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("no valid TAR entries found")))
        }

        return std.Result.Ok(std::Unit{})

        }
    }

func parse_tar_entries(archive : *mut TarArchive) : std::Result<std::Unit, ArchiveError> {
        unsafe {
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
            if(name.size() > 0 && name.at(name.size() - 1) == '/' as char) {
                is_dir = true
            }

            var entry : ArchiveEntry = ArchiveEntry {
                name: full_name,
                size: size,
                compressed_size: size,
                compression_method: 0,
                crc32: 0,
                is_directory: is_dir,
                offset: (pos + 512) as u64
            }

            archive.entries.push(entry)

            var data_blocks = (size + 511) / 512
            pos += 512 + (data_blocks * 512)
        }

        return std.Result.Ok(std::Unit{})

        }
    }

func tar_parse_octal(str : *mut string) : u64 {
        unsafe {
        var result : u64 = 0
        var i : size_t = 0
        while(i < str.size()) {
            var c = str.at(i)
            if(c >= '0' as char && c <= '7' as char) {
                result = result * 8 + ((c as u64) - ('0' as u64))
            }
            i += 1
        }
        return result

        }
    }

public func tar_entries(archive : *mut TarArchive) : *mut vector<ArchiveEntry> {
        unsafe {
        return &raw mut archive.entries

        }
    }

public func tar_entry_count(archive : *mut TarArchive) : size_t {
        unsafe {
        return archive.entries.size()

        }
    }

public func tar_find_entry(archive : *mut TarArchive, name : *char, output : *mut ArchiveEntry) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var i : size_t = 0
        while(i < archive.entries.size()) {
            var entry = archive.entries.get_ptr(i)
            if(str_equals_cstr(&raw mut entry.name, name)) {
                output.name = entry.name.copy()
                output.size = entry.size
                output.compressed_size = entry.compressed_size
                output.compression_method = entry.compression_method
                output.crc32 = entry.crc32
                output.is_directory = entry.is_directory
                output.offset = entry.offset
                return std.Result.Ok(std::Unit{})
            }
            i += 1
        }
        return std.Result.Err(ArchiveError.FileNotFound())

        }
    }

public func tar_read_entry(archive : *mut TarArchive, entry : *mut ArchiveEntry, output : *mut vector<u8>) : std::Result<std::Unit, ArchiveError> {
        unsafe {
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
    }

public func tar_read_file(archive : *mut TarArchive, name : *char) : std::Result<vector<u8>, ArchiveError> {
    var entry : ArchiveEntry
    var entry_result = tar_find_entry(archive, name, unsafe(&raw mut entry))
    if(entry_result is Result.Err) {
        return std.Result.Err(ArchiveError.FileNotFound())
    }
    var content = vector<u8>()
    var content_result = tar_read_entry(archive, unsafe(&raw mut entry), &raw mut content)
    if(content_result is Result.Err) {
        return std.Result.Err(ArchiveError.IoError(string("failed to read entry data")))
    }
    return std.Result.Ok(content)
}

public func tar_contains(archive : *mut TarArchive, name : *char) : bool {
    var entry = ArchiveEntry {
        name : string(),
        size : 0u,
        compressed_size : 0u,
        compression_method : 0u,
        crc32 : 0u,
        is_directory : false,
        offset : 0u
    }
    var r = tar_find_entry(archive, name, unsafe(&raw mut entry))
    return r is Result.Ok
}

public func tar_extract_entry(archive : *mut TarArchive, entry : *mut ArchiveEntry, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var full_path = unsafe(string(dest_dir))
        if(full_path.size() > 0 && full_path.at(full_path.size() - 1) != '/' as char) {
            full_path.append('/')
        }
        full_path.append_string(&entry.name)

        if(entry.is_directory) {
            fs::raw::create_dir_all(full_path.c_str())
            return std.Result.Ok(std::Unit{})
        }

        var content = vector<u8>()
        var content_result = tar_read_entry(archive, entry, &raw mut content)
        if(content_result is Result.Err) {
            return std.Result.Err(ArchiveError.IoError(string("failed to read entry for extraction")))
        }

        fs::raw::create_dir_all(full_path.c_str())

        var write_result = fs::raw::write_text_file(full_path.c_str(), content.data(), content.size())
        if(write_result is Result.Err) {
            return std.Result.Err(ArchiveError.IoError(string("failed to write file")))
        }

        return std.Result.Ok(std::Unit{})

        }
    }

public func tar_extract_all(archive : *mut TarArchive, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        fs::raw::create_dir_all(dest_dir)
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
    }

// ── from zip_write.ch ─────────────────────────────────────────────


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
        unsafe {
        var crc = crc32_compute(file_data, file_len)

        var entry : ZipWriteEntry = ZipWriteEntry {
            name: string(""),
            uncompressed_size: file_len as u32,
            compressed_size: file_len as u32,
            crc: crc,
            method: 0, // STORE
            local_header_offset: writer.data.size() as u32
        }
        entry.name.append_char_ptr(name)

        write_local_header(writer, name, file_len as u32, crc)

        var i : size_t = 0
        while(i < file_len) {
            writer.data.push(file_data[i])
            i += 1
        }

        writer.entries.push(entry)

        }
    }

public func zip_writer_add_bytes(writer : *mut ZipWriter, name : *char, data : *u8, data_len : size_t) {
    zip_writer_add_file(writer, name, data, data_len)
}

public func zip_writer_add_store(writer : *mut ZipWriter, name : *char, data : *u8, data_len : size_t) {
    zip_writer_add_file(writer, name, data, data_len)
}

func write_local_header(writer : *mut ZipWriter, name : *char, size : u32, crc : u32) {
        unsafe {
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
    }

public func zip_writer_close(writer : *mut ZipWriter) {
        unsafe {
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
                hptr2[46 + j as size_t] = entry.name.at(j as size_t) as u8
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
    }

public func zip_writer_to_bytes(writer : *mut ZipWriter) : *mut vector<u8> {
        unsafe {
        zip_writer_close(writer)
        return &raw mut writer.data

        }
    }

public func zip_writer_save(writer : *mut ZipWriter, path : *char) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        zip_writer_close(writer)

        var write_result = fs::raw::write_text_file(path, writer.data.data(), writer.data.size())
        if(write_result is Result.Err) {
            return std.Result.Err(ArchiveError.IoError(string("failed to write ZIP data")))
        }

        return std.Result.Ok(std::Unit{})

        }
    }

// ── from crc32.ch ─────────────────────────────────────────────


func crc32_table_entry(i : u32) : u32 {
    var crc = i;
    var j : u32 = 0;
    while(j < 8) {
        if((crc & 1u32) != 0) {
            crc = (crc >> 1) ^ 0xEDB88320u32;
        } else {
            crc = crc >> 1;
        }
        j += 1;
    }
    return crc;
}

public func crc32_compute(data : *u8, data_len : size_t) : u32 {
        unsafe {
        var crc : u32 = 0xFFFFFFFFu32;
        var i : size_t = 0;
        while(i < data_len) {
            var idx = (crc ^ (data[i] as u32)) & 0xFFu32;
            var table_val = crc32_table_entry(idx);
            crc = table_val ^ (crc >> 8);
            i += 1;
        }
        return crc ^ 0xFFFFFFFFu32;

        }
    }

public func crc32_update(crc : u32, data : *u8, data_len : size_t) : u32 {
        unsafe {
        var c = crc ^ 0xFFFFFFFFu32;
        var i : size_t = 0;
        while(i < data_len) {
            var idx = (c ^ (data[i] as u32)) & 0xFFu32;
            var table_val = crc32_table_entry(idx);
            c = table_val ^ (c >> 8);
            i += 1;
        }
        return c ^ 0xFFFFFFFFu32;

        }
    }

// ── from deflate.ch ─────────────────────────────────────────────



struct BitReader {
    var data : *u8
    var data_len : size_t
    var byte_pos : size_t
    var bit_buf : u32
    var bits_in_buf : int

    func init(d : *u8, d_len : size_t) : BitReader {
        return BitReader {
            data : d,
            data_len : d_len,
            byte_pos : 0,
            bit_buf : 0,
            bits_in_buf : 0
        }
    }

    func read_bits(&mut self, n : int) : u32 {
        unsafe {
            while(self.bits_in_buf < n) {
                if(self.byte_pos < self.data_len) {
                    self.bit_buf = self.bit_buf | ((self.data[self.byte_pos] as u32) << (self.bits_in_buf as u32))
                    self.byte_pos += 1
                    self.bits_in_buf += 8
                } else {
                    break
                }
            }
            var val = self.bit_buf & ((1u32 << (n as u32)) - 1u32)
            self.bit_buf = self.bit_buf >> (n as u32)
            self.bits_in_buf -= n
            return val

        }
    }

    func read_bits_reversed(&mut self, n : int) : u32 {
        var val = self.read_bits(n)
        var result : u32 = 0
        var i : int = 0
        while(i < n) {
            result = (result << 1) | (val & 1u32)
            val = val >> 1
            i += 1
        }
        return result
    }

    func align_to_byte(&mut self) {
        var extra = self.bits_in_buf % 8
        if(extra > 0) {
            self.bit_buf = self.bit_buf >> (extra as u32)
            self.bits_in_buf -= extra
        }
    }
}

const MAX_HUFFMAN_CODES = 288
const MAX_HUFFMAN_BITS = 15

struct HuffEntry {
    var code : u16
    var bits : u8
    var value : u16
}

@direct_init
struct HuffTable {
    var entries : vector<HuffEntry>
    var min_bits : int
    var max_bits : int
}

func build_huffman_table(lengths : *u8, num_codes : int, output : *mut HuffTable) : std::Result<std::Unit, ArchiveError> {
        unsafe {
        var bl_count : [MAX_HUFFMAN_BITS + 1]u32;
        var i : int = 0
        while(i <= MAX_HUFFMAN_BITS) {
            bl_count[i] = 0
            i += 1
        }

        i = 0
        while(i < num_codes) {
            var len = lengths[i] as int
            if(len > 0 && len <= MAX_HUFFMAN_BITS) {
                bl_count[len] += 1
            }
            i += 1
        }

        var max_bits : int = 0
        i = 1
        while(i <= MAX_HUFFMAN_BITS) {
            if(bl_count[i] > 0) { max_bits = i }
            i += 1
        }

        var next_code : [MAX_HUFFMAN_BITS + 1]u32
        var code : u32 = 0
        i = 1
        while(i <= MAX_HUFFMAN_BITS) {
            code = (code + bl_count[i - 1]) << 1
            next_code[i] = code
            i += 1
        }

        var entries = vector<HuffEntry>()
        var min_bits = MAX_HUFFMAN_BITS + 1
        if(max_bits > 0) { min_bits = max_bits }

        i = 0
        while(i < num_codes) {
            var len = lengths[i] as int
            if(len > 0) {
                var entry : HuffEntry
                entry.code = next_code[len] as u16
                entry.bits = len as u8
                entry.value = i as u16
                entries.push(entry)
                if(len < min_bits) { min_bits = len }
            }
            i += 1
        }

        output.entries = entries
        output.min_bits = min_bits
        output.max_bits = max_bits
        return std.Result.Ok(std::Unit{})

        }
    }

func decode_huffman(reader : *mut BitReader, table : *mut HuffTable) : u16 {
        unsafe {
        var code : u32 = 0
        var found_bits : int = 0
        var i : int = 0
        while(i < table.max_bits) {
            code = (code << 1) | reader.read_bits(1)
            found_bits += 1
            var j : size_t = 0
            while(j < table.entries.size()) {
                var e = table.entries.get_ptr(j)
                if(e.bits as int == found_bits && e.code as u32 == code) {
                    return e.value
                }
                j += 1
            }
        }
        return 0

        }
    }

public func deflate_decompress(input : *u8, input_len : size_t, output : *mut u8, output_capacity : size_t) : std::Result<size_t, ArchiveError> {
        unsafe {
        var reader = BitReader.init(input, input_len)
        var out_pos : size_t = 0
        var last_block = false

        while(!last_block) {
            var bfinal = reader.read_bits(1)
            var btype = reader.read_bits(2)

            if(bfinal == 1) { last_block = true }

            if(btype == 0) {
                reader.align_to_byte()
                if(reader.byte_pos + 4 > input_len) {
                    return std.Result.Err(ArchiveError.DecompressionFailed(string("truncated stored block header")))
                }
                var len = (reader.data[reader.byte_pos] as u16) | ((reader.data[reader.byte_pos + 1] as u16) << 8)
                reader.byte_pos += 4
                var i : u16 = 0
                while(i < len) {
                    if(out_pos >= output_capacity || reader.byte_pos >= input_len) {
                        return std.Result.Err(ArchiveError.DecompressionFailed(string("truncated stored block data")))
                    }
                    output[out_pos] = reader.data[reader.byte_pos]
                    out_pos += 1
                    reader.byte_pos += 1
                    i += 1
                }
            } else if(btype == 1 || btype == 2) {
                var lit_lens : [MAX_HUFFMAN_CODES]u8
                var dist_lens : [32]u8
                var num_lit_codes : int = 288
                var num_dist_codes : int = 0

                if(btype == 1) {
                    var i : int = 0
                    while(i < 144) { lit_lens[i] = 8; i += 1 }
                    while(i < 256) { lit_lens[i] = 9; i += 1 }
                    while(i < 280) { lit_lens[i] = 7; i += 1 }
                    while(i < 288) { lit_lens[i] = 8; i += 1 }
                    i = 0
                    while(i < 32) { dist_lens[i] = 5; i += 1 }
                    num_dist_codes = 32
                } else {
                    var hlit = reader.read_bits(5) as int + 257
                    var hdist = reader.read_bits(5) as int + 1
                    var hclen = reader.read_bits(4) as int + 4

                    num_lit_codes = hlit
                    num_dist_codes = hdist

                    var cl_order : [19]int = [16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15]
                    var cl_lens : [19]u8
                    var k : int = 0
                    while(k < 19) { cl_lens[k] = 0; k += 1 }
                    k = 0
                    while(k < hclen) {
                        cl_lens[cl_order[k]] = reader.read_bits(3) as u8
                        k += 1
                    }

                    var cl_table : HuffTable
                    var cl_result = build_huffman_table(&raw cl_lens[0], 19, unsafe(&raw mut cl_table))
                    if(cl_result is Result.Err) {
                        return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid code length Huffman table")))
                    }

                    var total = hlit + hdist
                    var idx = 0
                    while(idx < total) {
                        var sym = decode_huffman(&raw mut reader, unsafe(&raw mut cl_table)) as int
                        if(sym <= 15) {
                            if(idx < hlit) { lit_lens[idx] = sym as u8 }
                            else { dist_lens[idx - hlit] = sym as u8 }
                            idx += 1
                        } else if(sym == 16) {
                            var repeat = reader.read_bits(2) as int + 3
                            var prev_len : u8 = 0
                            if(idx > 0) {
                                if(idx - 1 < hlit) { prev_len = lit_lens[idx - 1] }
                                else { prev_len = dist_lens[idx - 1 - hlit] }
                            }
                            var r : int = 0
                            while(r < repeat && idx < total) {
                                if(idx < hlit) { lit_lens[idx] = prev_len }
                                else { dist_lens[idx - hlit] = prev_len }
                                idx += 1
                                r += 1
                            }
                        } else if(sym == 17) {
                            var repeat = reader.read_bits(3) as int + 3
                            var r : int = 0
                            while(r < repeat && idx < total) {
                                if(idx < hlit) { lit_lens[idx] = 0 }
                                else { dist_lens[idx - hlit] = 0 }
                                idx += 1
                                r += 1
                            }
                        } else if(sym == 18) {
                            var repeat = reader.read_bits(7) as int + 11
                            var r : int = 0
                            while(r < repeat && idx < total) {
                                if(idx < hlit) { lit_lens[idx] = 0 }
                                else { dist_lens[idx - hlit] = 0 }
                                idx += 1
                                r += 1
                            }
                        }
                    }
                }

                var lit_table : HuffTable
                var lit_result = build_huffman_table(&raw lit_lens[0], num_lit_codes, unsafe(&raw mut lit_table))
                if(lit_result is Result.Err) {
                    return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid literal/length Huffman table")))
                }

                var dist_table : HuffTable
                var dist_result = build_huffman_table(&raw dist_lens[0], num_dist_codes, unsafe(&raw mut dist_table))
                if(dist_result is Result.Err) {
                    return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid distance Huffman table")))
                }

                loop {
                        var sym = decode_huffman(&raw mut reader, unsafe(&raw mut lit_table))
                    if(sym < 256) {
                        if(out_pos >= output_capacity) {
                            return std.Result.Err(ArchiveError.DecompressionFailed(string("output buffer full")))
                        }
                        output[out_pos] = sym as u8
                        out_pos += 1
                    } else if(sym == 256) {
                        break
                    } else {
                        var len_extra_bits : [29]int = [0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0]
                        var len_base : [29]int = [3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,35,43,51,59,67,83,99,115,131,163,195,227,258]
                        var len_idx = (sym as int) - 257
                        if(len_idx < 0 || len_idx >= 29) {
                            return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid length code")))
                        }
                        var length = len_base[len_idx] + reader.read_bits(len_extra_bits[len_idx]) as int

                        var dist_sym = decode_huffman(&raw mut reader, unsafe(&raw mut dist_table))
                        var dist_extra_bits : [30]int = [0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13]
                        var dist_base : [30]int = [1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577]
                        var dist_idx = dist_sym as int
                        if(dist_idx < 0 || dist_idx >= 30) {
                            return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid distance code")))
                        }
                        var distance = dist_base[dist_idx] + reader.read_bits(dist_extra_bits[dist_idx]) as int

                        if(distance <= 0 || distance as size_t > out_pos) {
                            return std.Result.Err(ArchiveError.DecompressionFailed(string("invalid distance")))
                        }

                        var src_pos = out_pos - (distance as size_t)
                        var i : int = 0
                        while(i < length) {
                            if(out_pos >= output_capacity) {
                                return std.Result.Err(ArchiveError.DecompressionFailed(string("output buffer full during backreference")))
                            }
                            output[out_pos] = output[src_pos]
                            out_pos += 1
                            src_pos += 1
                            i += 1
                        }
                    }
                }
            } else {
                return std.Result.Err(ArchiveError.DecompressionFailed(string("reserved block type")))
            }
        }

        return std.Result.Ok(out_pos)

        }
    }

// ── from endian.ch ─────────────────────────────────────────────


public func read_u16_le(data : *u8, offset : size_t) : u16 {
        unsafe {
        return (data[offset] as u16) | ((data[offset + 1] as u16) << 8)

        }
    }

public func read_u32_le(data : *u8, offset : size_t) : u32 {
        unsafe {
        return (data[offset] as u32) | ((data[offset + 1] as u32) << 8) |
               ((data[offset + 2] as u32) << 16) | ((data[offset + 3] as u32) << 24)

        }
    }

public func read_u64_le(data : *u8, offset : size_t) : u64 {
        unsafe {
        return (data[offset] as u64) | ((data[offset + 1] as u64) << 8) |
               ((data[offset + 2] as u64) << 16) | ((data[offset + 3] as u64) << 24) |
               ((data[offset + 4] as u64) << 32) | ((data[offset + 5] as u64) << 40) |
               ((data[offset + 6] as u64) << 48) | ((data[offset + 7] as u64) << 56)

        }
    }

public func write_u16_le(out : *mut u8, offset : size_t, val : u16) {
        unsafe {
        out[offset] = (val & 0xFFu16) as u8
        out[offset + 1] = ((val >> 8) & 0xFFu16) as u8

        }
    }

public func write_u32_le(out : *mut u8, offset : size_t, val : u32) {
        unsafe {
        out[offset] = (val & 0xFFu32) as u8
        out[offset + 1] = ((val >> 8) & 0xFFu32) as u8
        out[offset + 2] = ((val >> 16) & 0xFFu32) as u8
        out[offset + 3] = ((val >> 24) & 0xFFu32) as u8

        }
    }

public func write_u64_le(out : *mut u8, offset : size_t, val : u64) {
        unsafe {
        out[offset] = (val & 0xFFu64) as u8
        out[offset + 1] = ((val >> 8) & 0xFFu64) as u8
        out[offset + 2] = ((val >> 16) & 0xFFu64) as u8
        out[offset + 3] = ((val >> 24) & 0xFFu64) as u8
        out[offset + 4] = ((val >> 32) & 0xFFu64) as u8
        out[offset + 5] = ((val >> 40) & 0xFFu64) as u8
        out[offset + 6] = ((val >> 48) & 0xFFu64) as u8
        out[offset + 7] = ((val >> 56) & 0xFFu64) as u8

        }
    }

} // end namespace raw

} // end namespace archive
