variant AlignmentTest42 {
    Large(s : std::string)
    Small(b : bool)
}

variant MixedAlignment42 {
    Text(s : std::string)
    Flag(b : bool)
    Number(n : int)
}

func test_variant_native() {

    test("variant alignment regression - 1 (small member after large member in different variants)", () => {
        // This variant has a large member (string is ~24-32 bytes) and a small member (bool is 1 byte)
        // The fix ensures that the GEP for the Bool member correctly accounts for the padding needed
        // to match the alignment of the largest member.
        var v = AlignmentTest42.Small(true)
        switch(v) {
            Small(b) => return b == true;
            default => return false;
        }
    })

    test("variant alignment regression - 2 (mixed sized members)", () => {

        var v1 = MixedAlignment42.Flag(true)
        var v2 = MixedAlignment42.Flag(false)
        var v3 = MixedAlignment42.Text(std::string("hello"))

        var ok1 = false
        switch(v1) { Flag(b) => { ok1 = b } default => {} }

        var ok2 = false
        switch(v2) { Flag(b) => { ok2 = !b } default => {} }

        var ok3 = false
        switch(v3) { Text(s) => { ok3 = s.equals_view("hello") } default => {} }

        return ok1 && ok2 && ok3
    })

}