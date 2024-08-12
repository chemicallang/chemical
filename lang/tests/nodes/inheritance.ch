import "../test.ch"

struct Animal {
    var a : long
    var b : long
}

func animal_sum(a : Animal*) : long {
    return a.a + a.b;
}

struct WalkingAnimal : Animal {
    var speed : long
}

struct Dog : WalkingAnimal {
    var c : long
    var d : long
}

func get_dog_sum(d : Dog*) : long {
    return d.c + d.d;
}

func test_single_inheritance() {
    test("passing base struct as a base struct pointer", () => {
        var a = Animal {
            a : 12,
            b : 13,
        }
        return animal_sum(&a) == 25;
    })
    test("passing a derived struct as a base class pointer", () => {
        var b = Dog {
            WalkingAnimal : WalkingAnimal {
                Animal : Animal {
                    a : 30,
                    b : 40
                },
                speed : 90
            },
            c : 40,
            d : 40
        }
        return animal_sum(&b) == 70 && get_dog_sum(&b) == 80;
    })
    test("can access base struct members from derived directly", () => {
        var b = Dog {
            WalkingAnimal : WalkingAnimal {
                Animal : Animal {
                    a : 30,
                    b : 40
                },
                speed : 90
            },
            c : 40,
            d : 20
        }
        return b.a + b.d == 50 && b.a + b.b == 70;
    })
    test("can access middle struct members from derived", () => {
        var b = Dog {
            WalkingAnimal : WalkingAnimal {
                Animal : Animal {
                    a : 30,
                    b : 40
                },
                speed : 90
            },
            c : 40,
            d : 20
        }
        return b.speed == 90
    })
}

func test_inheritance() {
    test_single_inheritance();
}