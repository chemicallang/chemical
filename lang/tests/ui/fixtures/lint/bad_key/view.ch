// UI-KEY-001: visible repeated node with no key.
func view(b : &mut ViewBuilder, items : &[Item]) {
    for(var item in items) {
        b.text(unknown_key(item), item.label, Style.new())
    }
}
