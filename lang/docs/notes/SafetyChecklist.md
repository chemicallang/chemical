This document is in development.

Tracking work that would be done early on in safety enforcement

Stack only, Single Threaded

- [ ] Enforce One mutable or many immutable borrows
- [ ] References cannot outlive the referent
  - [ ] Fail on reference being returned for a local variable
    - [ ] Check in ReturnStmt if the returned value becomes a reference, the `value` should be 
          allocated outside function frame (from a function parameter, contained inside a struct / array that is from outside function frame)
  - [ ] Fail on taking references in assignment, remove code for implicit reference taking in assignment, NOT supported because references are taken at construction time
    - references are immutable, you cannot change a reference
  - [ ] Fail on reference being stored in a struct/array/variant which would take the reference out of scope
    - [ ] When storing in a struct via struct value (even nested), it should be ensured that the lhs is of current scope
    - [ ] Cannot move a struct out of scope once a reference tied to current scope has been stored inside it
        - [ ] We'll do this by marking lhs (and checking moves) as tied to current scope once a current scope reference has been stored inside
    - [ ] References that came in function parameters are tired to scope of the function frame (valid inside it), they also only can be stored inside struct/array/variant of current scope variables only

Stack + Heap, Single Threaded

- [ ] Prevent references being stored in heap allocated objects unless they are being destroyed in or below scopes of referent

TODO