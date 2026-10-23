// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Zero-sized marker type that ties a generic parameter or lifetime to a type
 * without storing a value.
 *
 * This is the representation used by the safety/borrow-checking model for
 * variance and auto-trait participation, e.g.
 *
 *     struct Iter<'a, T> {
 *         var marker : std::PhantomData<&'a T>
 *     }
 *
 * The type has no fields, so it occupies zero bytes and needs no destructor.
 */
public struct PhantomData<T> {}

impl<T> Copy for PhantomData<T> {}

}
