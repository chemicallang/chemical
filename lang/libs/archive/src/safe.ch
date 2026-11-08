// Safe archive surface: owning handles, `&str`/`&[u8]`/`&path::Path` parameters,
// `Result` errors, and zip-slip-safe extraction. The legacy raw functions in
// `main.ch` / `zip_read.ch` / `tar_read.ch` remain available for callers that
// still need them; new code should use `Archive`.
public namespace archive {

using std::Result;
using std::string;
using std::vector;

// Owning archive handle. Both ZIP and TAR are supported; the format is sniffed
// by `open`/`from_bytes`.
public struct Archive {
    internal var inner : raw::Archive

    // ── construction ──────────────────────────────────────────────────────

    // Open a ZIP or TAR archive from a filesystem path.
    func open(path : &path::Path) : Result<Archive, ArchiveError> {
        var ps = path.as_string()
        var read = fs::read_entire_file(ps.c_str())
        if(read is Result.Err) {
            var Err(e) = read else unsafe { unreachable }
            return Result.Err(ArchiveError.FileNotFound())
        }
        var Ok(bytes) = read else unsafe { unreachable }
        var view : [u8] = &bytes
        return archive_safe_from_bytes(view.data(), view.size())
    }

    // Parse a ZIP or TAR archive from an in-memory byte buffer.
    func from_bytes(bytes : &[u8]) : Result<Archive, ArchiveError> {
        return archive_safe_from_bytes(bytes.data(), bytes.size())
    }

    // ── metadata ──────────────────────────────────────────────────────────

    func entry_count(&self) : size_t {
        if(self.inner.archive_type == 1) {
            return self.inner.zip_data.entries.size()
        }
        return self.inner.tar_data.entries.size()
    }

    // Return a read-only copy of the entry metadata. Directory entries keep
    // their trailing separator and `is_directory` flag.
    func entries(&self) : vector<ArchiveEntry> {
        var out = vector<ArchiveEntry>()
        var count = self.entry_count()
        var i : size_t = 0
        while(i < count) {
            out.push(archive_safe_copy_entry(&raw self.inner, i))
            i += 1
        }
        return out
    }

    // Copy one entry's metadata. Returns `IndexOutOfBounds` when `index` is
    // past the end.
    func entry(&self, index : size_t) : Result<ArchiveEntry, ArchiveError> {
        if(index >= self.entry_count()) {
            return Result.Err(ArchiveError.IndexOutOfBounds(index))
        }
        return Result.Ok(archive_safe_copy_entry(&raw self.inner, index))
    }

    // ── reads ─────────────────────────────────────────────────────────────

    // Read entry `index` into a newly allocated byte vector.
    func read_entry(&mut self, index : size_t) : Result<vector<u8>, ArchiveError> {
        if(index >= self.entry_count()) {
            return Result.Err(ArchiveError.IndexOutOfBounds(index))
        }
        var out = vector<u8>()
        var entry = archive_safe_entry_ptr_mut(&raw mut self.inner, index)
        if(self.inner.archive_type == 1) {
            var r = raw::zip_read_entry(&raw mut self.inner.zip_data, entry, &raw mut out)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
        } else {
            var r = raw::tar_read_entry(&raw mut self.inner.tar_data, entry, &raw mut out)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
        }
        return Result.Ok(out)
    }

    // Read an entry by name. Names use the archive's normal forward-slash
    // form (e.g. `"dir/file.txt"`).
    func read_file(&mut self, name : &str) : Result<vector<u8>, ArchiveError> {
        var index = archive_safe_find_index(&raw self.inner, name)
        if(index < 0) {
            return Result.Err(ArchiveError.FileNotFound())
        }
        return self.read_entry(index as size_t)
    }

    func contains(&self, name : &str) : bool {
        return archive_safe_find_index(&raw self.inner, name) >= 0
    }

    // ── extraction ────────────────────────────────────────────────────────

    // Extract every entry under `dest`. Entry names are validated before any
    // bytes are written: absolute paths, `..` components, and paths that pass
    // through an existing symlink are rejected with `ArchiveError.UnsafePath`.
    func extract(&mut self, dest : &path::Path) : Result<std::Unit, ArchiveError> {
        var base = dest.as_string()

        // Validate every name up front so a malicious entry cannot leave a
        // partially extracted tree behind.
        var count = self.entry_count()
        var i : size_t = 0
        while(i < count) {
            var e = archive_safe_copy_entry(&raw self.inner, i)
            if(!archive_safe_validate_name(&raw e.name)) {
                return Result.Err(ArchiveError.UnsafePath(e.name.copy()))
            }
            if(archive_safe_has_symlink(&raw base, &raw e.name)) {
                return Result.Err(ArchiveError.UnsafePath(e.name.copy()))
            }
            i += 1
        }

        var created = fs::create_dir_all(base.c_str())
        if(created is Result.Err) {
            return Result.Err(ArchiveError.IoError(string("failed to create destination directory")))
        }

        i = 0
        while(i < count) {
            var meta_result = self.entry(i)
            if(meta_result is Result.Err) {
                var Err(e) = meta_result else unsafe { unreachable }
                return Result.Err(e)
            }
            var meta = unsafe(unwrap_archive_entry(meta_result))
            var full = archive_safe_join(&raw base, &raw meta.name)

            if(meta.is_directory) {
                var dr = fs::create_dir_all(full.c_str())
                if(dr is Result.Err) {
                    return Result.Err(ArchiveError.IoError(string("failed to create directory")))
                }
                i += 1
                continue
            }

            var content_result = self.read_entry(i)
            if(content_result is Result.Err) {
                var Err(e) = content_result else unsafe { unreachable }
                return Result.Err(e)
            }
            var content = unsafe(unwrap_archive_bytes(content_result))

            var parent = fs::parent_path_view(full.to_view())
            if(parent.size() > 0) {
                var pr = fs::create_dir_all(parent.c_str())
                if(pr is Result.Err) {
                    return Result.Err(ArchiveError.IoError(string("failed to create parent directory")))
                }
            }

            var wr = fs::write_text_file(full.c_str(), content.data(), content.size())
            if(wr is Result.Err) {
                return Result.Err(ArchiveError.IoError(string("failed to write extracted file")))
            }
            i += 1
        }

        return Result.Ok(std::Unit{})
    }
}

// ── safe ZIP writer ───────────────────────────────────────────────────────

/// Safe in-memory ZIP writer. Entries are STORED (uncompressed) with names and
/// byte slices; `finish` produces the archive bytes and `save` writes them to
/// a path. The writer is single-shot: calling `finish`/`save` again returns the
/// same bytes without appending a second central directory.
@direct_init
public struct ZipWriter {
    internal var inner : raw::ZipWriter
    internal var finished : bool

    @make
    func make() : ZipWriter {
        return ZipWriter {
            inner : raw::ZipWriter {
                entries : vector<raw::ZipWriteEntry>(),
                data : vector<u8>()
            },
            finished : false
        }
    }

    public func new() : ZipWriter {
        return ZipWriter.make()
    }

    /// Add a stored entry. `name` is the archive-relative path (forward
    /// slashes); interior NUL bytes are rejected because the ZIP name encoding
    /// is C-string based.
    public func add_bytes(&mut self, name : &str, data : &[u8]) : Result<std::Unit, ArchiveError> {
        if(self.finished) {
            return Result.Err(ArchiveError.InvalidFormat(string("cannot add entries after finish")))
        }
        if(name.size() == 0) {
            return Result.Err(ArchiveError.InvalidFormat(string("empty entry name")))
        }
        var i : size_t = 0
        while(i < name.size()) {
            if(name.get(i) == 0 as char) {
                return Result.Err(ArchiveError.InvalidFormat(string("entry name contains NUL")))
            }
            i += 1
        }

        var name_buf = string()
        name_buf.append_view(name.to_view())
        name_buf.append(0 as char)
        raw::zip_writer_add_file(&raw mut self.inner, name_buf.c_str(), data.data(), data.size())
        self.finished = false
        return Result.Ok(std::Unit{})
    }

    /// Append the central directory (once) and return the ZIP bytes.
    public func finish(&mut self) : vector<u8> {
        if(!self.finished) {
            raw::zip_writer_close(&raw mut self.inner)
            self.finished = true
        }
        var out = vector<u8>()
        var i : size_t = 0
        while(i < self.inner.data.size()) {
            out.push(self.inner.data.get(i))
            i += 1
        }
        return out
    }

    public func save(&mut self, path : &path::Path) : Result<std::Unit, ArchiveError> {
        var bytes = self.finish()
        var ps = path.as_string()
        var written = fs::write_text_file(ps.c_str(), bytes.data(), bytes.size())
        if(written is Result.Err) {
            return Result.Err(ArchiveError.IoError(string("failed to write ZIP data")))
        }
        return Result.Ok(std::Unit{})
    }
}

// ── helpers ───────────────────────────────────────────────────────────────

func archive_safe_from_bytes(data : *u8, len : size_t) : Result<Archive, ArchiveError> {
    if(len == 0) {
        return Result.Err(ArchiveError.InvalidFormat(string("empty archive")))
    }

    var zip_archive = raw::Archive {
        zip_data : raw::ZipArchive { data : vector<u8>(), entries : vector<ArchiveEntry>(), data_loaded : false },
        tar_data : raw::TarArchive { data : vector<u8>(), entries : vector<ArchiveEntry>(), data_loaded : false },
        archive_type : 1,
        loaded : false
    }
    var zr = raw::open_zip_bytes(data, len, &raw mut zip_archive.zip_data)
    if(zr is Result.Ok) {
        zip_archive.loaded = true
        return Result.Ok(Archive { inner : zip_archive })
    }

    var tar_archive = raw::TarArchive { data : vector<u8>(), entries : vector<ArchiveEntry>(), data_loaded : false }
    unsafe {
        var i : size_t = 0
        while(i < len) {
            tar_archive.data.push(data[i])
            i += 1
        }
    }
    if(tar_archive.data.size() < 512) {
        return Result.Err(ArchiveError.InvalidFormat(string("not a recognized archive format")))
    }
    tar_archive.data_loaded = true
    var tr = raw::parse_tar_entries(&raw mut tar_archive)
    if(tr is Result.Err) {
        return Result.Err(ArchiveError.InvalidFormat(string("failed to parse TAR entries")))
    }
    if(tar_archive.entries.size() == 0) {
        return Result.Err(ArchiveError.InvalidFormat(string("no valid TAR entries found")))
    }

    return Result.Ok(Archive {
        inner : raw::Archive {
            zip_data : raw::ZipArchive { data : vector<u8>(), entries : vector<ArchiveEntry>(), data_loaded : false },
            tar_data : tar_archive,
            archive_type : 2,
            loaded : true
        }
    })
}

func archive_safe_entry_ptr_const(archive : *raw::Archive, index : size_t) : *ArchiveEntry {
    unsafe {
        if(archive.archive_type == 1) {
            return archive.zip_data.entries.get_ptr(index)
        }
        return archive.tar_data.entries.get_ptr(index)
    }
}

func archive_safe_copy_entry(archive : *raw::Archive, index : size_t) : ArchiveEntry {
    unsafe {
        var e = archive_safe_entry_ptr_const(archive, index)
        return ArchiveEntry {
            name : e.name.copy(),
            size : e.size,
            compressed_size : e.compressed_size,
            compression_method : e.compression_method,
            crc32 : e.crc32,
            is_directory : e.is_directory,
            offset : e.offset
        }
    }
}

func archive_safe_entry_ptr_mut(archive : *mut raw::Archive, index : size_t) : *mut ArchiveEntry {
    unsafe {
        if(archive.archive_type == 1) {
            return archive.zip_data.entries.get_ptr(index)
        }
        return archive.tar_data.entries.get_ptr(index)
    }
}

func archive_safe_find_index(archive : *raw::Archive, name : &str) : int {
    unsafe {
        var target = name.to_view()
        var count : size_t = 0
        if(archive.archive_type == 1) {
            count = archive.zip_data.entries.size()
        } else {
            count = archive.tar_data.entries.size()
        }
        var i : size_t = 0
        while(i < count) {
            var e = archive_safe_entry_ptr_const(archive, i)
            if(e.name.equals_view(&target)) {
                return i as int
            }
            i += 1
        }
        return -1
    }
}

// Reject absolute paths, Windows drive/UNC paths, NUL bytes, and `..`
// components. Empty names are rejected as unsafe.
func archive_safe_validate_name(name : *string) : bool {
    unsafe {
        if(name.size() == 0) {
            return false
        }
        var first = name.get(0)
        if(first == '/' as char || first == '\\' as char) {
            return false
        }
        if(name.size() >= 2 && name.get(1) == ':' as char) {
            return false
        }

        var i : size_t = 0
        var component_start : size_t = 0
        while(i <= name.size()) {
            var at_separator = (i == name.size()) || name.get(i) == '/' as char || name.get(i) == '\\' as char
            if(at_separator) {
                var clen = i - component_start
                if(clen == 2 && name.get(component_start) == '.' as char && name.get(component_start + 1) == '.' as char) {
                    return false
                }
                component_start = i + 1
            } else if(name.get(i) == 0 as char || name.get(i) == ':' as char) {
                // NUL is never a valid filename byte; ':' would allow a
                // Windows drive/alternate-stream shape on cross-platform
                // extraction.
                return false
            }
            i += 1
        }
        return true
    }
}

func archive_safe_is_symlink(p : *string) : bool {
    unsafe {
        var r = fs::metadata(p.c_str())
        if(r is Result.Err) {
            return false
        }
        var Ok(m) = r else unsafe { unreachable }
        return m.is_symlink
    }
}

// True when `base` or any existing prefix of `base/name` is a symlink.
func archive_safe_has_symlink(base : *string, name : *string) : bool {
    unsafe {
        if(archive_safe_is_symlink(base)) {
            return true
        }
        var current = base.copy()
        var i : size_t = 0
        var component_start : size_t = 0
        while(i <= name.size()) {
            var at_separator = (i == name.size()) || name.get(i) == '/' as char || name.get(i) == '\\' as char
            if(at_separator) {
                if(i > component_start) {
                    if(current.size() > 0) {
                        var last = current.get(current.size() - 1)
                        if(last != '/' as char && last != '\\' as char) {
                            current.append('/')
                        }
                    }
                    var j = component_start
                    while(j < i) {
                        current.append(name.get(j))
                        j += 1
                    }
                    if(archive_safe_is_symlink(&raw current)) {
                        return true
                    }
                }
                component_start = i + 1
            }
            i += 1
        }
        return false
    }
}

func archive_safe_join(base : *string, name : *string) : string {
    unsafe {
        var full = base.copy()
        if(full.size() > 0) {
            var last = full.get(full.size() - 1)
            if(last != '/' as char && last != '\\' as char) {
                full.append('/')
            }
        }
        full.append_string(&*name)
        return full
    }
}

func unwrap_archive_entry(r : Result<ArchiveEntry, ArchiveError>) : ArchiveEntry {
    var Ok(v) = r else unsafe { unreachable }
    return v
}

func unwrap_archive_bytes(r : Result<vector<u8>, ArchiveError>) : vector<u8> {
    var Ok(v) = r else unsafe { unreachable }
    return v
}

} // end namespace archive
