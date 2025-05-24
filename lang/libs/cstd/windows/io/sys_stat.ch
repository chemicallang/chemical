public enum FileTypeBits : uint {

    /**
     * Bitmask for the file type bitfields.
     */
    Mask = _S_IFMT,
    /**
     *  Directory.
     */
    Directory = _S_IFDIR,
    /**
     *  Regular file.
     */
    Regular = _S_IFREG,
    /**
     *  Character device.
     */
    CharDev = _S_IFCHR,
    /**
     *  FIFO (named pipe).
     */
    FIFO = _S_IFIFO,

};