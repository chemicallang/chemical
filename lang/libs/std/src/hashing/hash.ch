public interface Eq {

    func equals(&self, other : &self) : bool

}

public interface Hashable {

    func hash(&self) : uint

}