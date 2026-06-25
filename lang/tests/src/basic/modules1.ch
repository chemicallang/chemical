
func test_downloaded_module() {
    test("downloaded module example_sum works", () => {
        return example_sum(120, 4) == 124
    })
}

func test_modules_import() {
    test("can import files from modules using '@' prefix", () => {
        return extern_imported_sum(10, 10) == 40;
    })
}