// UI-PANIC-001: panic in UI app/view code.
func view(b : &mut ViewBuilder) {
    var value = maybe()
    if(value.is_none()) { panic("missing value") }
}
