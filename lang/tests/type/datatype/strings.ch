import "../../test.ch"

func test_strings() {
    test("check string indexing works", () => {
        var x = "true";
        return x[0] == 't' && x[1] == 'r' && x[2] == 'u' && x[3] == 'e';
    })
}