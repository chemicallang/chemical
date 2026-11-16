// Fixture for `scripts/instantiation-names-test.sh`.
//
// This module instantiates generic types the `std` module also instantiates
// (`vector<T>`, `Option<T>`, `unordered_map<K, V>`) with *different* arguments,
// and it reaches the same instantiations `std` uses internally. That is the
// situation the content-derived instantiation names exist for: with positional
// names (`__cgs__4`) two different types in two modules shared one name, and the
// C compiler kept whichever body it saw first.
//
// The script builds this twice and checks that the emitted name set is identical
// and that no name is defined twice.

using std::Option;
using std::Result;
using std::string;
using std::unordered_map;
using std::vector;

// A generic function, so a `__cfg_` instantiation name (its own hash) is in play
// next to the `__cgs_` ones of its parameter types.
func <T> item_count(items : &vector<T>) : u64 {
    return items.size() as u64
}

func fill_names() : vector<string> {
    var names = vector<string>()
    names.push(string("alpha"))
    names.push(string("beta"))
    return names
}

public func main() : int {
    // `vector<u8>`: std instantiates `vector<string>` and `vector<u8>` for its own
    // purposes, this module instantiates both too.
    var bytes = vector<u8>()
    var one : u8 = 1
    bytes.push(one)
    if(bytes.size() != 1u) { return 1 }

    // `Option<u8>` from `get`: std reaches other `Option<T>` instantiations, so
    // the same generic carries different arguments in the two modules.
    var slot = bytes.get(0u)
    if(slot.is_none()) { return 2 }

    // `vector<string>`: the instantiation std also owns, reached from here.
    var names = fill_names()
    if(names.size() != 2u) { return 3 }
    var first = names.get_ptr(0u)
    if(unsafe(first.size()) != 5u) { return 4 }

    // `unordered_map<int, int>`: an instantiation std does not reach, with its
    // own generic arguments in the nested map type.
    var counts = unordered_map<int, int>()
    counts.insert(1, 1)
    if(counts.size() != 1u) { return 5 }

    // a `__cfg_` instantiation of a generic function
    var ints = vector<int>()
    if(item_count<int>(&mut ints) != 0u) { return 6 }

    return 0
}
