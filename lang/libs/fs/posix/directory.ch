func posix_mkdir(pathname : *char, mode : uint) : int {
        unsafe {
        return unsafe(mkdir(pathname, mode))

        }
    }

public namespace fs {



} // namespace fs
