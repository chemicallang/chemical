
using namespace std;

func test_time_types() {

    // ---- Duration tests --------------------------------------------------

    test("Duration::from_secs", () => {
        var d = std::chrono::Duration::from_secs(5)
        return d.as_secs() == 5
    })

    test("Duration::from_millis", () => {
        var d = std::chrono::Duration::from_millis(1500)
        return d.as_secs() == 1 && d.subsec_nanos() == 500000000
    })

    test("Duration::from_micros", () => {
        var d = std::chrono::Duration::from_micros(1000000)
        return d.as_secs() == 1 && d.subsec_nanos() == 0
    })

    test("Duration::from_nanos", () => {
        var d = std::chrono::Duration::from_nanos(2500000000)
        return d.as_secs() == 2 && d.subsec_nanos() == 500000000
    })

    test("Duration add", () => {
        var a = std::chrono::Duration::from_secs(10)
        var b = std::chrono::Duration::from_secs(20)
        var c = a.add(&b)
        return c.as_secs() == 30 && c.subsec_nanos() == 0
    })

    test("Duration add with nanos overflow", () => {
        var a = std::chrono::Duration::from_millis(500)
        var b = std::chrono::Duration::from_millis(800)
        var c = a.add(&b)
        return c.as_secs() == 1 && c.subsec_nanos() == 300000000
    })

    test("Duration sub", () => {
        var a = std::chrono::Duration::from_secs(50)
        var b = std::chrono::Duration::from_secs(30)
        var c = a.sub(&b)
        return c.as_secs() == 20
    })

    test("Duration neg", () => {
        var a = std::chrono::Duration::from_secs(42)
        var b = a.neg()
        return b.as_secs() == -42
    })

    test("Duration equals", () => {
        var a = std::chrono::Duration::from_secs(10)
        var b = std::chrono::Duration::from_secs(10)
        return a.equals(&b)
    })

    test("Duration cmp", () => {
        var a = std::chrono::Duration::from_secs(5)
        var b = std::chrono::Duration::from_secs(10)
        return a.cmp(&b) == -1 && b.cmp(&a) == 1
    })

    test("Duration mul_i64", () => {
        var a = std::chrono::Duration::from_secs(3)
        var b = a.mul_i64(4)
        return b.as_secs() == 12
    })

    test("Duration div_i64", () => {
        var a = std::chrono::Duration::from_secs(100)
        var b = a.div_i64(5)
        return b.as_secs() == 20
    })

    test("Duration abs", () => {
        var d = std::chrono::Duration::from_secs(-7)
        var a = d.abs()
        return a.as_secs() == 7
    })

    test("Duration as_millis", () => {
        var d = std::chrono::Duration::from_secs(2)
        return d.as_millis() == 2000
    })

    test("Duration as_micros", () => {
        var d = std::chrono::Duration::from_secs(1)
        return d.as_micros() == 1000000
    })

    test("Duration as_nanos", () => {
        var d = std::chrono::Duration::from_secs(1)
        return d.as_nanos() == 1000000000
    })

    test("Duration from_parts normalizes", () => {
        var d = std::chrono::Duration::from_parts(2, 2500000000)
        return d.as_secs() == 4 && d.subsec_nanos() == 500000000
    })

    // ---- Instant tests (basic) ------------------------------------------

    test("Instant default is zero", () => {
        var inst : std::chrono::Instant = std::chrono::Instant()
        // Default should be zero; Instant exposes secs/nanos as fields
        return inst.secs == 0 && inst.nanos == 0
    })

    // ---- SystemTime tests (basic) ---------------------------------------

    test("SystemTime default is zero", () => {
        var st = std::chrono::SystemTime()
        return st.as_unix_epoch_secs() == 0
    })

    test("SystemTime from_unix_epoch", () => {
        var t = std::chrono::SystemTime::from_unix_epoch(1000000)
        return t.as_unix_epoch_secs() == 1000000
    })

    test("SystemTime add_duration", () => {
        var t = std::chrono::SystemTime::from_unix_epoch(1000)
        var dur = std::chrono::Duration::from_secs(500)
        var later = t.add_duration(&dur)
        return later.as_unix_epoch_secs() == 1500
    })

    test("SystemTime sub_duration", () => {
        var t = std::chrono::SystemTime::from_unix_epoch(1000)
        var dur = std::chrono::Duration::from_secs(500)
        var earlier = t.sub_duration(&dur)
        return earlier.as_unix_epoch_secs() == 500
    })

    test("SystemTime duration_since", () => {
        var a = std::chrono::SystemTime::from_unix_epoch(100)
        var b = std::chrono::SystemTime::from_unix_epoch(200)
        var d = b.duration_since(&a)
        return d.as_secs() == 100
    })

    test("SystemTime equals", () => {
        var a = std::chrono::SystemTime::from_unix_epoch(42)
        var b = std::chrono::SystemTime::from_unix_epoch(42)
        return a.equals(&b)
    })

}
