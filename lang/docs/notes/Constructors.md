```

// initialzing fields directly
struct InitializeFields {

    var a : int
    var b : int
    
    @make
    func make() {
        init {
           a = 10
           b = 20
        }
    }
    
    @make
    func make2() {
        init {
            a = 10
            // compiler throws error that 'b' wasn't initialized in the init block
        }
    }
    
    @make
    func make3() {
        // compiler throws error that no init block is present
        // because one of a or b doesn't have a default value
        a = 10
    }
    
    @make
    func make4() {
        init {
            a = 10
            b = 20
        }
        // initializes 'a', 'b' again
        // but intent is clear, looking at syntax, you did a = 10 twice, so you should expect that
        a = 10 
        b = 20
    }
    
    @make
    func make5(value : int) {
         init {
            a = value
            b = value
         }
    }

}

struct DefaultFieldValues {
    
    var a : int = 10
    var b : int = 20
    
    @make
    func make() {
        // automatically sets a, b to their default values
        // no init block required because all fields have default values
    }
   
    @make
    func make2() {
        // however this causes double initialization of variable 'a'        
        // first a is assigned 10, then 30        
        // the intent is not clear here, you wrote a = 30 once
        // maybe we should force all direct assignments to happen inside init block
        a = 30
    }
    
    @make
    func make3() {
        // correct way to assign a = 30
        init {
            a = 30
            // a is initialized once with 30
            // b is initialized once with 20 (default)
        }
    }
    
}

struct DefaultConstructable {
    
    // InitializeFields has a default constructor
    var x : InitializeFields
    
    @make
    func make() {
        // automatically initializes InitializeFields by calling its default constructor
        // init block not required because all fields are default initializable
    }
    
    @make
    func make() {
        // this causes double initialization
        // maybe this could also be prevented by forcing all direct assignments to happen inside init block
        x = InitializeFields(99)
    }
    
    @make
    func make() {
        init {
            // x is initialized once with the custom contructor call
            // default constructor call is NOT made
            x = InitializeFields(99)
        }
    }
    
    
}

struct Deletable {
    @make
    func make() {
        
    }
    @make
    func make2(value : int) {
        // dummy constructor
    }
    @delete
    func delete(&self) {
        // dummy destructor
    }
}

struct DeletableConstructable {
    
    // InitializeFields has a default constructor
    var x : Deletable
    
    @make
    func make() {
        // automatically initializes Deletable by calling its default constructor
        // init block not required because all fields are default initializable
    }
    
    @make
    func make() {
        // this causes double initialization
        // since this is assignment, 'x' is first destructed and then constructed (its not placement new)
        // maybe this could also be prevented by forcing all direct assignments to happen inside init block
        x = Deletable()
    }
    
    @make
    func make() {
        init {
            // x is initialized once with the custom contructor call
            // default constructor call is NOT made
            x = Deletable()
        }
    }
    
}

namespace container {
    struct Deletable {
        @make
        func make() {
            
        }
        @make
        func make2(value : int) {
            // dummy constructor
        }
        @delete
        func delete(&self) {
            // dummy destructor
        }
    }
}

struct InheritInitialize : container::Deletable {

    @make
    func make() {
        // init block not required, because inherited Deletable has a default constructor
        // which would be called automatically
        // there's no syntax to call constructor manually for inherited struct without init block
        // so this constructor is safe
    }
    
    @make
    func make2() {
        init {
            // intiialized the Deletable inherited struct once with custom constructor call
            // call to default constructor is NOT made
            container::Deletable = container::Deletable(324)
        }
    }

}

```