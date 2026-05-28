// this is a generic struct present in module B
// if we consider main -> B -> C
// its using a generic struct present in module C
// the monomorphization will take place in main though
public struct ExposedGenSecond<T : Copy> {
    var value : T
    func give(&self) : T {
        var value = ExposedGenThird<T> { value : value }
        return value.give() + 1;
    }
}