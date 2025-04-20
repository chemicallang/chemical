public namespace fs {

    public func mkdir(pathname : *char) : int {
        if(def.windows) {
            return _mkdir(pathname)
        } else {
            return mkdir(pathname, PermissionMode.S_IRWXU)
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

    public enum CopyOptions {
        None        = 0,
        Recursive   = 1 << 0,
        Overwrite   = 1 << 1,
        SkipExisting = 1 << 2
    };

    @comptime public const COPY_NONE = CopyOptions.None;
    @comptime public const COPY_RECURSIVE = CopyOptions.Recursive;
    @comptime public const COPY_OVERWRITE = CopyOptions.Overwrite;
    @comptime public const COPY_SKIP_EXISTING = CopyOptions.SkipExisting;

    public func path_exists(path : *char) : int {
        var st : Stat;
        return stat(path, &st) == 0;
    }

    public func is_directory(path : *char) : int {
        var st : Stat;
        if (stat(path, &st) != 0) return 0;
        if(def.windows) {
            return (st.st_mode & S_IFDIR) != 0;
        } else {
            return S_ISDIR(st.st_mode);
        }
    }

    public func copy_file(src : *char, dest : *char, options : CopyOptions) : int {
        if (!(options & COPY_OVERWRITE) && path_exists(dest)) {
            if (options & COPY_SKIP_EXISTING) return 0;
            fprintf(stderr, "File exists: %s\n", dest);
            return -1;
        }
        if(def.windows) {
            if (!CopyFileA(src, dest, !(options & COPY_OVERWRITE))) {
                fprintf(stderr, "CopyFileA failed: %s -> %s\n", src, dest);
                return -1;
            }
            return 0;
        } else {
            const in_fd = open(src, O_RDONLY, 0);
            if (in_fd < 0) { perror("open src"); return -1; }

            const out_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0o644);
            if (out_fd < 0) { perror("open dest"); close(in_fd); return -1; }

            var buf : char[8192];
            var n : ssize_t;
            while (true) {
                n = read(in_fd, buf, sizeof(buf))
                if(n <= 0) {
                    break;
                }
                if (write(out_fd, buf, n) != n) {
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
    }

    public func copy_directory(src_dir : *char, dest_dir : *char, options : CopyOptions) : int {

        if (!is_directory(src_dir)) {
            fprintf(stderr, "Not a directory: %s\n", src_dir);
            return -1;
        }

        if(def.windows) {
            if (!path_exists(dest_dir)) {
                if (!CreateDirectoryA(dest_dir, NULL)) {
                    fprintf(stderr, "CreateDirectoryA failed: %s\n", dest_dir);
                    return -1;
                }
            }
            var dir = opendir(src_dir);
        } else {
            if (!path_exists(dest_dir) && mkdir(dest_dir, 0755) != 0) {
                perror("mkdir dest");
                return -1;
            }
            var dir = opendir(src_dir);
        }

        if (!dir) { perror("opendir"); return -1; }

        var entry : *mut dirent;
        var src_path : char[1024]
        var dst_path : char[1024]

        while(true) {

            entry = readdir(dir)
            if(entry == null) {
                break;
            }

            var name = entry.d_name;

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, name);
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dest_dir, name);

            var st : Stat;
            if (stat(src_path, &st) != 0) {
                perror("Stat");
                closedir(dir);
                return -1;
            }

            if(def.windows) {
                var is_dir = (st.st_mode & S_IFDIR) != 0;
            } else {
                var is_dir = S_ISDIR(st.st_mode);
            }

            if (is_dir) {
                if (options & COPY_RECURSIVE) {
                    if (copy_directory(src_path, dst_path, options) != 0) {
                        closedir(dir);
                        return -1;
                    }
                }
            } else {
                if (copy_file(src_path, dst_path, options) != 0) {
                    closedir(dir);
                    return -1;
                }
            }
        }

        closedir(dir);
        return 0;
    }

**/

}