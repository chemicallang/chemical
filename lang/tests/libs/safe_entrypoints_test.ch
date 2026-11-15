using namespace std;
using path::Path;

// Compile-coverage gate for the *safe* entry point of every library.
//
// The helpers below are never called. Chemical type-checks and lowers every
// function body, so referencing an entry point here fails the build when the
// safe door is unreachable (declared as a namespace-level bare `func`, i.e.
// internal), was renamed, or no longer type-checks.
//
// That failure mode is real: the `*safe*.ch` files in audio/image/font declare
// their helpers as namespace-level bare `func`, which is *internal* — the
// public door is the struct method (`Audio.load`, `Image.load`, `Font.load`).
// Referencing the wrong door here is exactly the mistake this file catches, so
// every entry below must name the public one.
//
// Keep this list to the safe door only. The `::raw` door is covered by
// `scripts/safety-raw-surface-audit.py`; runtime behaviour is covered by the
// per-library tests.

// ---- filesystem -----------------------------------------------------------
func _entry_fs() : int {
    var p = Path.new("x")
    var a = fs::read(&p)
    var b = fs::read_to_string(&p)
    var c = fs::read_dir(&p)
    var d = fs::create_dir_all(&p)
    var e = fs::metadata(&p)
    var f = fs::exists(&p)
    var g = fs::remove_file(&p)
    var buf : [4]u8 = [1, 2, 3, 4]
    var h = fs::write(&p, buf)
    return 0
}

// ---- path -----------------------------------------------------------------
func _entry_path() : int {
    var p = Path.new("a/b.txt")
    var q = p.join(&Path.new("c"))
    var n = q.normalize()
    var v = q.as_view()
    var s = q.to_path_buf()
    return 0
}

// ---- media / archives -----------------------------------------------------
func _entry_media() : int {
    var p = Path.new("x.png")
    var img = image::Image.load(&p)
    var aud = audio::Audio.load(&p)
    var arc = archive::Archive.open(&p)
    var bytes : [4]u8 = [0, 0, 0, 0]
    var fnt = font::Font.load(bytes)
    return 0
}

// ---- environment ---------------------------------------------------------
func _entry_env() : int {
    var v = environment::var("PATH")
    var d = environment::current_dir()
    var h = environment::home_dir()
    return 0
}

// ---- data -----------------------------------------------------------------
func _entry_data() : int {
    var s = string("{}")
    var v = s.to_view()
    var j = json::parse(v)
    var re = regex::try_compile("a")
    var m = regex::compile("a")
    var dt = datetime::DateTime.parse("%Y", "2026")
    var now = datetime::DateTime.now()
    var u = uuid::v4()
    var hx = encoding::hex_encode(v.as_bytes())
    var sum = crypto::sha256(v.as_bytes())
    var jf = json::stringify(&JsonValue.String(string("ok")))
    return 0
}

@test
func safe_entrypoints_are_reachable(env : &mut TestEnv) {
    // Everything above compiled, which is the assertion. Calling the pure-data
    // helper proves this file is live (a stale/renamed file would silently stop
    // covering) and that the data entry points also run.
    if(_entry_data() != 0) {
        env.error("safe data entry points")
    }
}
