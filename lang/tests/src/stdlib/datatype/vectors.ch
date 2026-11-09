
using namespace std;

struct VecConcreteChild {
    var a : int
    var b : int
}

struct VecEmpty {}

struct VecEmptyDestructible {
    @delete
    func delete(&self) {
        unsafe { vec_destruct_called++ }
    }
}

var vec_destruct_called = 0;

struct VecDestructibleChild {
    var i : int
    func copy(&self) : VecDestructibleChild {
        unsafe {
            return { i : i }
    
        }
    }
    @delete
    func delete(&self) {
        unsafe {
            vec_destruct_called++
    
        }
    }
}

func test_vectors() {
        unsafe {
        test("vector of ints work", () => {
            var v = vector<int>();
            v.push(10);
            v.push(20);
            return v.size() == 2 && v.at(0) == 10 && v.at(1) == 20
        })
        test("vector of longs work", () => {
            var v = vector<long>();
            v.push(10);
            v.push(20);
            return v.size() == 2 && v.at(0) == 10 && v.at(1) == 20
        })
        test("zero-sized vector elements work", () => {
            var v = vector<VecEmpty>();
            v.push(VecEmpty {});
            v.push(VecEmpty {});
            if(v.size() != 2) { return false }
            v.remove(0);
            if(v.size() != 1) { return false }
            v.clear();
            if(!v.empty()) { return false }
            v.resize(4);
            if(v.size() != 4) { return false }
            v.resize(1);
            return v.size() == 1;
        })
        vec_destruct_called = 0;
        test("zero-sized vector destructors run per element", () => {
            var v = vector<VecEmptyDestructible>();
            v.push(VecEmptyDestructible {});
            v.push(VecEmptyDestructible {});
            v.clear();
            if(vec_destruct_called != 2) { return false }
            v.push(VecEmptyDestructible {});
            v.push(VecEmptyDestructible {});
            v.remove(0);
            return vec_destruct_called == 3 && v.size() == 1;
        })
        test("can remove last element from vector", () => {
            var v = vector<int>();
            v.push(10);
            v.push(20);
            v.remove_last();
            return v.size() == 1 && v.at(0) == 10;
        })
        test("can remove first element from vector", () => {
            var v = vector<int>();
            v.push(10);
            v.push(20);
            v.remove(0);
            return v.size() == 1 && v.at(0) == 20;
        })
        test("vectors work even when pushing large amount of items", () => {
            var v = vector<int>();
            var i = 10;
            var sum = 0;
            while(i < 300) {
                v.push(10);
                sum += 10;
                i += 10;
            }
            var act_sum = 0;
            i = 0;
            while(i < v.size()) {
                act_sum += v.at(i as size_t)
                i++;
            }
            return sum == act_sum;
        })
        test("can change vector elements at index", () => {
            var v = vector<int>();
            v.push(10);
            v.push(20);
            v.push(30);
            v.push(40);
            v.push(50);
            v.set(0, 50);
            v.set(1, 40);
            v.set(2, 30);
            v.set(3, 20);
            v.set(4, 10);
            return v.at(0) == 50 && v.at(1) == 40 && v.at(2) == 30 && v.at(3) == 20 && v.at(4) == 10;
        })
        test("get_mut mutates an element through a mutable vector", () => {
            var v = vector<int>();
            v.push(10);
            v.push(20);
            *v.at_mut(0) = 99;
            return v.at(0) == 99 && v.at(1) == 20;
        })
        vec_destruct_called = 0;
        test("vector set destroys the replaced destructible element", () => {
            var v = vector<VecDestructibleChild>();
            v.push(VecDestructibleChild { i : 99 })
            v.set(0, VecDestructibleChild { i : 29 })
            if(vec_destruct_called != 1) { return false }
            if(v.size() != 1 || v.get_ptr(0).i != 29) { return false }
            return true
        })

        test("struct types can be stored in vector", () => {
            var v = vector<VecConcreteChild>();
            v.push(VecConcreteChild { a : 40, b : 50 })
            v.push(VecConcreteChild { a : 10, b : 3 })
            v.push(VecConcreteChild { a : 67, b : 232 })
            const first = v.get_ptr(0)
            const second = v.get_ptr(1)
            const third = v.get_ptr(2)
            return first.a == 40 && first.b == 50 && second.a == 10 && second.b == 3 && third.a == 67 && third.b == 232
        })
        vec_destruct_called = 0;
        test("vector destructs it's destructible struct - 1", () => {
            var v = vector<VecDestructibleChild>();
            v.push(VecDestructibleChild { i : 99 })
            v.push(VecDestructibleChild { i : 29 })
            v.push(VecDestructibleChild { i : 49 })
            v.remove(0)
            const first = v.get_ptr(0)
            const second = v.get_ptr(1)
            return v.size() == 2 && vec_destruct_called == 1 && first.i == 29 && second.i == 49;
        })
        vec_destruct_called = 0;
        test("vector destructs it's destructible struct - 2", () => {
            var v = vector<VecDestructibleChild>();
            v.push(VecDestructibleChild { i : 99 })
            v.push(VecDestructibleChild { i : 29 })
            v.push(VecDestructibleChild { i : 49 })
            v.remove_last();
            const first = v.get_ptr(0)
            const second = v.get_ptr(1)
            return v.size() == 2 && vec_destruct_called == 1 && first.i == 99 && second.i == 29;
        })
        vec_destruct_called = 0;
        test("vector destructs it's destructible struct - 3", () => {
            if(true) {
                var v = vector<VecDestructibleChild>();
                v.push(VecDestructibleChild { i : 99 })
                v.push(VecDestructibleChild { i : 29 })
                v.push(VecDestructibleChild { i : 49 })
            }
            return vec_destruct_called == 3;
        })

        }
    }