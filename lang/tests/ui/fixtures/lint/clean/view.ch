// clean fixture: keys, no panic, no raw pointers in app/view code.
func view(b : &mut ViewBuilder, items : &[Item]) {
    for(var item, i in items) {
        var k = key_of("row", i as u64)
        b.text(k, item.label, Style.new())
    }
}
