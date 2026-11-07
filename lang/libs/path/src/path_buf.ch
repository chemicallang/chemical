// Owning path value with safe, allocation-backed component operations.
public namespace path {

using std::string;
using std::string_view;

@direct_init
public struct PathBuf {
    internal var value : string

    @make
    func from(value : &str) {
        return PathBuf {
            value : unsafe(string(value.data(), value.size()))
        }
    }

    func as_view(&self) : string_view {
        return self.value.to_view()
    }

    func as_string(&self) : string {
        return self.value.copy()
    }

    func size(&self) : size_t {
        return self.value.size()
    }

    func empty(&self) : bool {
        return self.value.size() == 0
    }

    func basename(&self) : PathBuf {
        var buf : [4096]char
        var r = path::basename(self.value.c_str(), &raw mut buf[0], 4096)
        if(r is std::Result.Err) { return PathBuf { value : string() } }
        return PathBuf { value : unsafe(string.make_no_len(&raw mut buf[0])) }
    }

    func dirname(&self) : PathBuf {
        var buf : [4096]char
        var r = path::dirname(self.value.c_str(), &raw mut buf[0], 4096)
        if(r is std::Result.Err) { return PathBuf { value : string() } }
        return PathBuf { value : unsafe(string.make_no_len(&raw mut buf[0])) }
    }

    func extension(&self) : PathBuf {
        var buf : [4096]char
        var r = path::extension(self.value.c_str(), &raw mut buf[0], 4096)
        if(r is std::Result.Err) { return PathBuf { value : string() } }
        return PathBuf { value : unsafe(string.make_no_len(&raw mut buf[0])) }
    }

    func stem(&self) : PathBuf {
        var buf : [4096]char
        var r = path::stem(self.value.c_str(), &raw mut buf[0], 4096)
        if(r is std::Result.Err) { return PathBuf { value : string() } }
        return PathBuf { value : unsafe(string.make_no_len(&raw mut buf[0])) }
    }

    func join(&self, other : &str) : PathBuf {
        var made = std::cstring_make(other)
        if(made is std::Result.Err) { return PathBuf { value : string() } }
        var Ok(other_c) = made else unsafe { unreachable }
        var buf : [4096]char
        var r = path::join(self.value.c_str(), unsafe(other_c.as_ptr()), &raw mut buf[0], 4096)
        if(r is std::Result.Err) { return PathBuf { value : string() } }
        return PathBuf { value : unsafe(string.make_no_len(&raw mut buf[0])) }
    }

    func normalize(&self) : PathBuf {
        var buf : [4096]char
        var r = path::normalize(self.value.c_str(), &raw mut buf[0], 4096)
        if(r is std::Result.Err) { return PathBuf { value : string() } }
        return PathBuf { value : unsafe(string.make_no_len(&raw mut buf[0])) }
    }

    func is_absolute(&self) : bool {
        return unsafe(path::is_absolute(self.value.c_str()))
    }

    func has_root(&self) : bool {
        return unsafe(path::has_root(self.value.c_str()))
    }
}

public type Path = PathBuf;

} // namespace path
