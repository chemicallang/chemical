// UI-RAW-001: raw pointer in app/view code without an unsafe host boundary.
func view(b : &mut ViewBuilder, value : int) {
    var p = &raw mut value
}
