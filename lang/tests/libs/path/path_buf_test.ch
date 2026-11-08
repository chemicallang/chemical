using namespace std;

@test
func test_pathbuf_components(env : &mut TestEnv) {
    var p = path::PathBuf.from("/usr/bin/gcc")
    var name = p.file_name()
    if(name is Option.None) { env.error("PathBuf.file_name missing"); return }
    var Some(n) = name else unsafe { unreachable }
    if(!n.as_view().equals(string_view("gcc"))) {
        env.error("PathBuf.file_name")
    }
    var dir = p.parent()
    if(dir is Option.None) { env.error("PathBuf.parent missing"); return }
    var Some(d) = dir else unsafe { unreachable }
    if(!d.as_view().equals(string_view("/usr/bin"))) {
        env.error("PathBuf.parent")
    }
    if(!p.is_absolute()) {
        env.error("PathBuf.is_absolute")
    }
    if(p.has_root() != true) {
        env.error("PathBuf.has_root")
    }
    if(!p.as_path().as_str().is_some()) {
        env.error("PathBuf.as_str")
    }
}

@test
func test_pathbuf_join_normalize(env : &mut TestEnv) {
    var p = path::PathBuf.from("/usr")
    var other = path::Path.new("bin/gcc")
    var joined = p.join(&other)
    if(!joined.as_view().equals(string_view("/usr/bin/gcc"))) {
        env.error("PathBuf.join")
    }
    var n = path::PathBuf.from("/usr/bin/../lib/./file.txt").normalize()
    if(n is Result.Err) {
        env.error("PathBuf.normalize failed")
    } else {
        var Ok(normalized) = n else unsafe { unreachable }
        if(!normalized.as_view().equals(string_view("/usr/lib/file.txt"))) {
            env.error("PathBuf.normalize")
        }
    }
    var ext = path::PathBuf.from("file.tar.gz").extension()
    if(ext is Option.None) {
        env.error("PathBuf.extension missing")
    } else {
        var Some(e) = ext else unsafe { unreachable }
        if(!e.as_view().equals(string_view("gz"))) {
            env.error("PathBuf.extension")
        }
    }
    var stem = path::PathBuf.from("file.tar.gz").stem()
    if(stem is Option.None) {
        env.error("PathBuf.stem missing")
    } else {
        var Some(s) = stem else unsafe { unreachable }
        if(!s.as_view().equals(string_view("file.tar"))) {
            env.error("PathBuf.stem")
        }
    }
}

@test
func test_path_borrowed_view(env : &mut TestEnv) {
    var p = path::PathBuf.from("/tmp/example.txt")
    var view = p.as_path()
    if(view.as_view().size() != p.size()) { env.error("Path view size") }
    if(view.file_name() is Option.None) { env.error("Path.file_name") }
    var ext = view.extension()
    if(ext is Option.None) {
        env.error("Path.extension missing")
    } else {
        var Some(e) = ext else unsafe { unreachable }
        if(!e.as_view().equals(string_view("txt"))) { env.error("Path.extension") }
    }
    var root = path::Path.new("/")
    if(root.file_name() is Option.Some) { env.error("root should have no file name") }
    if(root.parent() is Option.Some) { env.error("root should have no parent") }
    if(!root.has_root()) { env.error("root has_root") }
}

@test
func test_path_rejects_bad_native_form(env : &mut TestEnv) {
    var p = path::PathBuf.from("ok/path")
    if(p.has_nul()) { env.error("clean path reports NUL") }
    if(p.native_ptr() == null) { env.error("PathBuf should cache native form") }
}
