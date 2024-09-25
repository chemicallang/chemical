import "../../test.ch"

func test_html() {
    test("library is integrated", () => {
        var h = #html { 33 }
        return h == 33;
    })
}