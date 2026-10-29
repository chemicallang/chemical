using namespace std;

struct MemDropCounter {
    var id : int
    @delete func delete(&mut self) {
        unsafe {
            mem_drops = mem_drops + 1
    
        }
    }
}

var mem_drops = 0

func test_mem() {
        unsafe {
        test("mem::replace returns the old value and moves the source", () => {
            mem_drops = 0
            {
                var value = MemDropCounter { id : 1 }
                var old = std::mem.replace(&mut value, MemDropCounter { id : 2 })
                if(old.id != 1 || value.id != 2) {
                    return false
                }
            }
            return mem_drops == 2
        })
        test("mem::replace works with heap strings", () => {
            var value = string("old")
            var old = std::mem.replace(&mut value, string("new"))
            var expected_old = string("old")
            var expected_new = string("new")
            return old.equals(&expected_old) && value.equals(&expected_new)
        })
        test("std::replace compatibility alias moves the value", () => {
            var value = string("old")
            var old = std::replace(&mut value, string("new"))
            var expected_old = string("old")
            var expected_new = string("new")
            return old.equals(&expected_old) && value.equals(&expected_new)
        })
        test("mem::swap exchanges values", () => {
            var a = 10
            var b = 20
            std::mem.swap(&mut a, &mut b)
            return a == 20 && b == 10
        })
        test("mem::swap exchanges owned values once", () => {
            mem_drops = 0
            {
                var a = MemDropCounter { id : 3 }
                var b = MemDropCounter { id : 4 }
                std::mem.swap(&mut a, &mut b)
                if(a.id != 4 || b.id != 3) {
                    return false
                }
            }
            return mem_drops == 2
        })
        test("mem::forget suppresses the destructor", () => {
            mem_drops = 0
            {
                var value = MemDropCounter { id : 5 }
                std::mem.forget(value)
            }
            return mem_drops == 0
        })

        }
    }
