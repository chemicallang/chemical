import "@cstd/stdio.ch"

// TODO these functions should not be public
// we just require that their linkage is public
// the posix one could be called by the user

if(def.windows) {

    @extern
    public func _mkdir(path : *char) : int

} else {

    public type mode_t = uint;

    // leaving this enum as non-public so it's not accessible
    enum PermissionMode : uint {
        S_IRWXU = 0700,  /* Read, write, and execute/search by owner */
        S_IRUSR = 0400,  /* Read permission, owner */
        S_IWUSR = 0200,  /* Write permission, owner */
        S_IXUSR = 0100,  /* Execute/search permission, owner */
        S_IRWXG = 0070,  /* Read, write, and execute/search by group */
        S_IRGRP = 0040,  /* Read permission, group */
        S_IWGRP = 0020,  /* Write permission, group */
        S_IXGRP = 0010,  /* Execute/search permission, group */
        S_IRWXO = 0007,  /* Read, write, and execute/search by others */
        S_IROTH = 0004,  /* Read permission, others */
        S_IWOTH = 0002,  /* Write permission, others */
        S_IXOTH = 0001   /* Execute/search permission, others */
    }

    @extern
    public func mkdir(pathname : *char,  mode : mode_t) : int;

    alias posix_mkdir = mkdir;

}

public namespace fs {

    public func mkdir(pathname : *char) : int {
        if(def.windows) {
            return _mkdir(pathname)
        } else {
            return posix_mkdir(pathname, PermissionMode.S_IRWXU)
        }
    }

    func find_last_pos_of_or(str : *char, len : size_t, value : char, value2 : char) : int {
        if(len == 0) return -1;
        var i : int = len as int - 1;
        while(i >= 0) {
            // printf("checking %c against %c and %c \n", str[i], value, value2)
            if(str[i] == value || str[i] == value2) {
                return i;
            }
            // printf("found not\n");
            i--;
        }
        return -1;
    }

    public func parent_path(str : std::string_view) : std::string {
        var final = std::string()
        var pos : int
        if(def.windows) {
            pos = find_last_pos_of_or(str.data(), str.size(), '\\', '/')
        } else {
            pos = find_last_pos_of_or(str.data(), str.size(), '/', '/')
        }
        if(pos > 0) {
            final.append_with_len(str.data(), pos + 1)
        }
        return final;
    }

    public enum WriteResult : int {
        Success = 0,
        ErrOpen = -1,
        ErrWrite = -2,
        ErrClose = -3
    };

    // Writes the string 'data' to the file at 'filepath'.
    // Returns a WriteResult enum value.
    public func write_to_file(filepath : *char, data : *char) : WriteResult {
        const file = fopen(filepath, "w");
        if (file == null) {
            return WriteResult.ErrOpen;
        }
        if (fputs(data, file) == EOF) {
            fclose(file);
            return WriteResult.ErrWrite;
        }
        if (fclose(file) != 0) {
            return WriteResult.ErrClose;
        }
        return WriteResult.Success;
    }

}