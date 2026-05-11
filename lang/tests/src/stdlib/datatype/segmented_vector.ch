func test_segmented_vector() {
    test("segmented_vector stores and retrieves values across chunks", () => {
        var s = std::segmented_vector<int>()
        s.push(10)
        s.push(20)
        s.push(30)
        s.push(40)
        s.push(50)
        s.push(60)

        var first = s.get_ptr(0)
        var fourth = s.get_ptr(3)
        var fifth = s.get_ptr(4)
        var sixth = s.get_ptr(5)

        if(first == null || fourth == null || fifth == null || sixth == null) {
            return false
        }

        return s.size() == 6 && *first == 10 && *fourth == 40 && *fifth == 50 && *sixth == 60
    })

    test("segmented_vector reports empty and size", () => {
        var s = std::segmented_vector<int>()
        if(!s.empty() || s.size() != 0) {
            return false
        }
        s.push(1)
        s.push(2)
        return !s.empty() && s.size() == 2
    })

    test("segmented_vector clear destroys stored values", () => {
        var s = std::segmented_vector<int>()
        s.push(1)
        s.push(2)
        s.push(3)
        s.clear()
        return s.empty() && s.size() == 0
    })
}
