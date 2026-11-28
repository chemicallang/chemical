// UI-PANIC-001: unchecked value access in UI app/view code.
func view(b : &mut ViewBuilder, value : Option<int>) {
    var v = value.take_unchecked()
    b.text(key("value"), "x", Style.new())
}
