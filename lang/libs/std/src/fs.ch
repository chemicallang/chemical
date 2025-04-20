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
        if(def.windows) {
            return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
        } else {
            var st : Stat;
            return stat(path, &st) == 0;
        }
    }

    public func is_directory(path : *char) : int {
        if(def.windows) {
            const attributes = GetFileAttributesA(path);
            return (attributes != INVALID_FILE_ATTRIBUTES) && (attributes & FILE_ATTRIBUTE_DIRECTORY);
        } else {
            var st : Stat;
            if (stat(path, &st) != 0) return 0;
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
            // Note: CopyFileA's bFailIfExists parameter handles the overwrite logic.
            // The check above is redundant if relying solely on CopyFileA's parameter,
            // but useful for the SkipExisting logic.
            // CopyFileA fails if the destination exists and bFailIfExists is TRUE.
            // So, if !COPY_OVERWRITE, we want bFailIfExists to be TRUE.
            if (!CopyFileA(src, dest, !(options & COPY_OVERWRITE))) {
                // Check GetLastError() for more specific error information if needed
                // DWORD err = GetLastError();
                fprintf(stderr, "CopyFileA failed: %s -> %s\n", src, dest); // Consider adding error code
                return -1;
            }
            return 0;
        } else {
            const in_fd = open(src, O_RDONLY, 0);
            if (in_fd < 0) { perror("open src"); return -1; }

            // Use O_EXCL with O_CREAT if COPY_OVERWRITE is not set, for atomic check and create
            // However, the path_exists check already handles SkipExisting and initial overwrite check.
            // O_TRUNC ensures if it exists and OVERWRITE is set, it's truncated.
            const out_fd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0o644);
            if (out_fd < 0) { perror("open dest"); close(in_fd); return -1; }

            var buf : char[8192];
            var n : ssize_t;

            while (true) {
                n = read(in_fd, buf, sizeof(buf));
                if(n <= 0) { // 0 for EOF, -1 for error
                    if (n < 0) perror("read");
                    break; // Exit loop on error or EOF
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
            // Check if the loop exited due to a read error
            if (n < 0) return -1;

            return 0;
        }
    }

    public func copy_directory(src_dir : *char, dest_dir : *char, options : CopyOptions) : int {

        if (!is_directory(src_dir)) {
            fprintf(stderr, "Not a directory: %s\n", src_dir);
            return -1;
        }

        // Create destination directory if it doesn't exist
        if(def.windows) {
            if (!path_exists(dest_dir)) {
                if (!CreateDirectoryA(dest_dir, NULL)) { // Assumes CreateDirectoryA is available
                    fprintf(stderr, "CreateDirectoryA failed: %s\n", dest_dir);
                    // Consider adding GetLastError() info
                    return -1;
                }
            } else if (!is_directory(dest_dir)) {
                fprintf(stderr, "Destination exists but is not a directory: %s\n", dest_dir);
                return -1;
            }
        } else {
            if (!path_exists(dest_dir)) {
                 // Assumes mkdir is available
                if (mkdir(dest_dir, 0755) != 0) {
                    perror("mkdir dest");
                    return -1;
                }
            } else if (!is_directory(dest_dir)) {
                 fprintf(stderr, "Destination exists but is not a directory: %s\n", dest_dir);
                 return -1;
            }
        }

        if(def.windows) {

            var find_data : WIN32_FIND_DATAA; // Assumes WIN32_FIND_DATAA is defined

            // Append "\\*" to list directory contents on Windows
            // Need to handle path concatenation carefully, maybe use a temporary buffer
            // or a path joining helper function.
            // For now, let's assume a helper `join_paths_windows` exists.
            // Example: join_paths_windows(temp_path, sizeof(temp_path), src_dir, "*");
            // And then use temp_path in FindFirstFileA.
            // Or, more directly for FindFirstFile, append \* to the directory name.
            // Let's create a path like "src_dir\*"
            var search_path : char[1024]; // Assuming 1024 is sufficient
            snprintf(search_path, sizeof(search_path), "%s\\*", src_dir);

            var hFind : HANDLE = FindFirstFileA(search_path, &find_data);

            if (hFind == INVALID_HANDLE_VALUE) { // Assumes INVALID_HANDLE_VALUE is defined
                // GetLastError() could provide more info
                fprintf(stderr, "FindFirstFileA failed: %s\n", src_dir);
                return -1; // Indicates error during directory opening
            }

            // Loop through directory contents
            do {
                var name = &find_data.cFileName[0]; // Assumes cFileName is the correct field

                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                    continue;

                // Construct full paths for source and destination
                var src_path : char[1024];
                var dst_path : char[1024];
                // Assuming a cross-platform path joining helper or using snprintf with platform separator
                // For simplicity, let's assume snprintf with appropriate separator
                // Use backslash on Windows
                snprintf(src_path, sizeof(src_path), "%s\\%s", src_dir, name);
                snprintf(dst_path, sizeof(dst_path), "%s\\%s", dest_dir, name);


                // Check if the entry is a directory using Windows attributes
                var is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY); // Assumes FILE_ATTRIBUTE_DIRECTORY is defined

                if (is_dir) {
                    if (options & COPY_RECURSIVE) {
                        if (copy_directory(src_path, dst_path, options) != 0) {
                            FindClose(hFind); // Close the handle before returning
                            return -1;
                        }
                    }
                } else {
                    if (copy_file(src_path, dst_path, options) != 0) {
                         FindClose(hFind); // Close the handle before returning
                        return -1;
                    }
                }
            } while (FindNextFileA(hFind, &find_data) != 0); // Assumes FindNextFileA is available

            var last_error : DWORD = GetLastError();
            if (last_error != ERROR_NO_MORE_FILES) { // Check if the loop terminated due to an error other than end of files
                 // GetLastError() could provide more info
                fprintf(stderr, "FindNextFileA failed\n"); // Consider adding error code
                FindClose(hFind); // Close the handle
                return -1; // Indicate error during iteration
            }

            FindClose(hFind); // Assumes FindClose is available
            return 0; // Success
        } else {
            // POSIX Implementation
            var dir = opendir(src_dir); // Assumes opendir is available
            if (!dir) { perror("opendir"); return -1; }

            var entry : *mut dirent; // Assumes dirent is defined
            var src_path : char[1024];
            var dst_path : char[1024];

            while(true) {
                entry = readdir(dir); // Assumes readdir is available
                if(entry == null) {
                     // Check errno to differentiate between end of directory and error
                     // For simplicity, assuming end of directory if errno is not set or is 0 after loop.
                     // A robust check would involve clearing errno before the call and checking it after.
                    break; // Exit loop on error or end of directory
                }

                var name = entry.d_name; // Assumes d_name is the correct field

                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                    continue;

                 // Construct full paths for source and destination
                // Use forward slash on POSIX
                snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, name);
                snprintf(dst_path, sizeof(dst_path), "%s/%s", dest_dir, name);


                // Check if the entry is a directory using stat and S_ISDIR
                var st : Stat; // Assumes Stat is defined/aliased appropriately
                if (stat(src_path, &st) != 0) { // Assumes stat is available/wrapped
                    perror("Stat");
                    closedir(dir); // Close the handle before returning
                    return -1;
                }

                var is_dir = S_ISDIR(st.st_mode); // Assumes S_ISDIR is defined/available

                if (is_dir) {
                    if (options & COPY_RECURSIVE) {
                        if (copy_directory(src_path, dst_path, options) != 0) {
                            closedir(dir); // Close the handle before returning
                            return -1;
                        }
                    }
                } else {
                    if (copy_file(src_path, dst_path, options) != 0) {
                        closedir(dir); // Close the handle before returning
                        return -1;
                    }
                }
            }

            closedir(dir); // Assumes closedir is available
             // Check errno after loop to see if readdir failed
             // if (errno != 0) { perror("readdir"); return -1; } // More robust error checking

            return 0; // Success
        }
    }

**/

}