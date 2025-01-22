import "/test.ch"
import "@submod/check.ch"


func test_modules_import() {
    test("can import files from modules using '@' prefix", () => {
        return extern_imported_sum(10, 10) == 40;
    })
}