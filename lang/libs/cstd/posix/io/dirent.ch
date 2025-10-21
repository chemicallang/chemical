// add or replace these typedefs (if not already present)
type ino_t = u64;
type off_t = i64;
type __d_reclen_t = u16;
type __d_type_t = u8;
const PATH_MAX_BUF = 4096; // already used elsewhere

/**
 * @struct dirent
 * @brief Directory entry data.
 */
public struct dirent {
    var d_ino    : ino_t;               // inode number
    var d_off    : off_t;               // offset to next dirent
    var d_reclen : __d_reclen_t;       // length of this record
    var d_type   : __d_type_t;         // type of file
    // name buffer (flexible in C; represent as a reasonably large array)
    var d_name   : [4096]char;
}

/** @brief Opaque directory stream type. */
// TODO implementation defined, maybe we should not introduce its implementation
// since its only used as a pointer
public struct DIR {

}

/**
 * @brief Open a directory stream.
 * @param name Path to the directory.
 * @return DIR* on success, NULL on error.
 */
@extern
public func opendir(name : *char) : *mut DIR

/**
 * @brief Read next directory entry.
 * @param dirp Directory stream.
 * @return Pointer to dirent on success, or NULL at end/error.
 */
@extern
public func readdir(dirp : *mut DIR) : *mut dirent

/**
 * @brief Close a directory stream.
 * @param dirp Directory stream to close.
 * @return 0 on success, –1 on error.
 */
@extern
public func closedir(dirp : *mut DIR) : int