struct CapLambDestrCounter {
    var counter : *mut int
    @delete
    func delete(&self) {
        *counter = *counter + 1;
    }
}

struct CapLambContainer {
    var fun : std::function<() => void>
}

func test_capturing_lambda_destruction() {
    test("capturing lambda destructor is called in var init", () => {
        var counter = 0;
        var m = CapLambDestrCounter { counter : &counter }
        if(counter == 0) {
            var lambda : std::function<() => void> = |m|() => {

            }
        }
        return counter == 1;
    })
    test("capturing lambda destructor is called when contained in struct", () => {
        var counter = 0
        var m = CapLambDestrCounter { counter : &counter }
        if(counter == 0) {
            var capLamb = CapLambContainer {
                fun : |m|() => {

                }
            }
        }
        return counter == 1;
    })
}