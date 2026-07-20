public namespace archive {

using std::Result;
using std::string;
using std::vector;

@direct_init
public struct Archive {
    var zip_data : ZipArchive
    var tar_data : TarArchive
    var archive_type : int
    var loaded : bool
}

public func open(path : *char) : std::Result<Archive, ArchiveError> {
    var archive : Archive
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
    if(archive.archive_type == 1) {
        return zip_entries(&raw mut archive.zip_data)
    } else {
        return tar_entries(&raw mut archive.tar_data)
    }
}

public func entry_count(archive : *mut Archive) : size_t {
    if(archive.archive_type == 1) {
        return zip_entry_count(&raw mut archive.zip_data)
    } else {
        return tar_entry_count(&raw mut archive.tar_data)
    }
}

public func contains(archive : *mut Archive, name : *char) : bool {
    if(archive.archive_type == 1) {
        return zip_contains(&raw mut archive.zip_data, name)
    } else {
        return tar_contains(&raw mut archive.tar_data, name)
    }
}

public func read(archive : *mut Archive, name : *char) : std::Result<vector<u8>, ArchiveError> {
    if(archive.archive_type == 1) {
        return zip_read_file(&raw mut archive.zip_data, name)
    } else {
        return tar_read_file(&raw mut archive.tar_data, name)
    }
}

public func extract(archive : *mut Archive, dest : *char) : std::Result<std::Unit, ArchiveError> {
    if(archive.archive_type == 1) {
        return zip_extract_all(&raw mut archive.zip_data, dest)
    } else {
        return tar_extract_all(&raw mut archive.tar_data, dest)
    }
}

public func extract_entry(archive : *mut Archive, name : *char, dest_dir : *char) : std::Result<std::Unit, ArchiveError> {
    var entry : ArchiveEntry
    if(archive.archive_type == 1) {
        var entry_result = zip_find_entry(&raw mut archive.zip_data, name, &raw mut entry)
        if(entry_result is Result.Err) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("entry not found")))
        }
        return zip_extract_entry(&raw mut archive.zip_data, &raw mut entry, dest_dir)
    } else {
        var entry_result = tar_find_entry(&raw mut archive.tar_data, name, &raw mut entry)
        if(entry_result is Result.Err) {
            return std.Result.Err(ArchiveError.InvalidFormat(string("entry not found")))
        }
        return tar_extract_entry(&raw mut archive.tar_data, &raw mut entry, dest_dir)
    }
}

} // end namespace archive
