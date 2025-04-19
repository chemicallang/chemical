if(!def.windows) {

/**
 * @struct dirent
 * @brief Directory entry data.
 */
public struct dirent {
    var d_ino : ulong;        /**< File serial number */
    var d_name : char[256]; /**< Null‑terminated filename */
};

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
public func opendir(name : *char) : *mut DIR

/**
 * @brief Read next directory entry.
 * @param dirp Directory stream.
 * @return Pointer to dirent on success, or NULL at end/error.
 */
public func readdir(dirp : *mut DIR) : *mut dirent

/**
 * @brief Close a directory stream.
 * @param dirp Directory stream to close.
 * @return 0 on success, –1 on error.
 */
public func closedir(dirp : *mut DIR) : int

}