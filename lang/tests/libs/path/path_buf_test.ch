using namespace std;

@test
func test_pathbuf_components(env : &mut TestEnv) {
    var p = path::PathBuf.from("/usr/bin/gcc")
    if(!p.basename().as_view().equals(string_view("gcc"))) {
        env.error("PathBuf.basename")
    }
    if(!p.dirname().as_view().equals(string_view("/usr/bin"))) {
        env.error("PathBuf.dirname")
    }
    if(!p.is_absolute()) {
        env.error("PathBuf.is_absolute")
    }
    if(p.has_root() != true) {
        env.error("PathBuf.has_root")
    }
}

@test
func test_pathbuf_join_normalize(env : &mut TestEnv) {
    var p = path::PathBuf.from("/usr")
    var joined = p.join("bin/gcc")
    if(!joined.as_view().equals(string_view("/usr/bin/gcc"))) {
        env.error("PathBuf.join")
    }
    var n = path::PathBuf.from("/usr/bin/../lib/./file.txt").normalize()
    if(!n.as_view().equals(string_view("/usr/lib/file.txt"))) {
        env.error("PathBuf.normalize")
    }
    if(!path::PathBuf.from("file.tar.gz").extension().as_view().equals(string_view(".gz"))) {
        env.error("PathBuf.extension")
    }
    if(!path::PathBuf.from("file.tar.gz").stem().as_view().equals(string_view("file.tar"))) {
        env.error("PathBuf.stem")
    }
}
