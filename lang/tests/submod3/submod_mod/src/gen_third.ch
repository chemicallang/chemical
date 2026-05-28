// this is a generic struct present in module C
// if we consider main -> B -> C
// in this case B will contain another generic struct that will use this struct
// the monomorphization will take place in main though
public struct ExposedGenThird<T : Copy> {
    var value : T
    func give(&self) : T {
        return value;
    }
}