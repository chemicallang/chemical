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

/**

    enum CopyOptions {
        None        = 0,
        Recursive   = 1 << 0,
        Overwrite   = 1 << 1,
        SkipExisting = 1 << 2
    };

    func path_exists(path : *char) : int {
        struct stat st;
        return stat(path, &st) == 0;
    }

    func is_directory(path : *char) : int {
        struct stat st;
        return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
    }

    func copy_file(src : *char, dest : *char, options : CopyOptions) : int {
        if (!(options & CopyOptions.Overwrite) && path_exists(dest)) {
            if (options & CopyOptions.SkipExisting)
                return 0;
            fprintf(stderr, "File already exists: %s\n", dest);
            return -1;
        }

        int in_fd = open(src, O_RDONLY);
        if (in_fd < 0) {
            perror("open src");
            return -1;
        }

        int out_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out_fd < 0) {
            perror("open dest");
            close(in_fd);
            return -1;
        }

        char buffer[8192];
        ssize_t bytes;
        while ((bytes = read(in_fd, buffer, sizeof(buffer))) > 0) {
            if (write(out_fd, buffer, bytes) != bytes) {
                perror("write");
                close(in_fd);
                close(out_fd);
                return -1;
            }
        }

        close(in_fd);
        close(out_fd);
        return 0;
    }

    public func copy_directory(src_dir : *char, dest_dir : *char, options : CopyOptions) : int {

        if (!is_directory(src_dir)) {
            fprintf(stderr, "Source is not a directory: %s\n", src_dir);
            return -1;
        }

        if (!path_exists(dest_dir)) {
            if (mkdir(dest_dir, 0755) != 0) {
                perror("mkdir");
                return -1;
            }
        }

        const dir = opendir(src_dir);
        if (!dir) {
            perror("opendir");
            return -1;
        }

        struct dirent *entry;
        char src_path[1024], dest_path[1024];

        while ((entry = readdir(dir))) {
            const name = entry->d_name;

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                continue;
            }

            snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, name);
            snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, name);

            struct stat st;
            if (stat(src_path, &st) != 0) {
                perror("stat");
                closedir(dir);
                return -1;
            }

            if (S_ISDIR(st.st_mode)) {
                if (options & CopyOptions.Recursive) {
                    if (copy_directory(src_path, dest_path, options) != 0) {
                        closedir(dir);
                        return -1;
                    }
                }
            } else if (S_ISREG(st.st_mode)) {
                if (copy_file(src_path, dest_path, options) != 0) {
                    closedir(dir);
                    return -1;
                }
            } // Could handle symlinks, etc., here
        }

        closedir(dir);
        return 0;
    }

**/

}