// A nested `{ ... }` block is a scope: locals declared inside it must be
// destroyed when the block ends, not at function exit (and not leaked). A
// codegen bug previously lowered blocks as plain statement lists, so a
// destructor-bearing local inside a block was never dropped.

var scope_block_drops : int = 0

struct ScopeBlockDestructible {
    var v : int

    @delete
    func delete(&mut self) {
        scope_block_drops = scope_block_drops + 1
    }
}

func test_scope_block_destruction() {

    test("nested block destroys its local at block end", () => {
        scope_block_drops = 0
        {
            var d = ScopeBlockDestructible { v : 1 }
            if(d.v != 1) { return false }
        }
        return scope_block_drops == 1
    })

    test("nested block destroys all of its locals", () => {
        scope_block_drops = 0
        {
            var a = ScopeBlockDestructible { v : 1 }
            var b = ScopeBlockDestructible { v : 2 }
            if(a.v + b.v != 3) { return false }
        }
        return scope_block_drops == 2
    })

    test("nested block local is destroyed before later outer code runs", () => {
        scope_block_drops = 0
        var before = 0
        {
            var d = ScopeBlockDestructible { v : 5 }
            before = scope_block_drops
        }
        // still zero inside the block, one after it ends
        return before == 0 && scope_block_drops == 1
    })
}
