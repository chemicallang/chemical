import "./importable/imported.ch";
print("imported thing is : ");
println(imp);
var v : int;
v = 9;
print("The output is : ");
println(v);
func add(a : int, b : int) : int {
    return a + b;
}
interface IBase {
    var x : int
    func something(a : int) : int;
}
struct X {
    var x = 5
    var y = 6
}
var zz = add(5, 5);
print("Function Call ");
print(zz);
println(' ');
var x : int = 1;
if(x == 1) {
    x += 8 + 2;
}
println(x);
enum Some {
    First,
    Second
}
print("Enum value First : ");
println(Some.First);
print("Enum value Second : ");
println(Some.Second);
print("While Loop (breakon:5): ");
var w = 0;
while(w < 10) {
    print(w);
    if(w == 5){
        break;
    }
    w++;
}
println(' ');
print("Do While Loop (missing:5): ");
var dw = 0;
do {
    dw++;
    if(dw == 5) {
        continue;
    }
    print(dw);
} while(dw < 10);
println(' ');
print("For Loop (missing:6): ");
for(var j = 0; j < 10; j++) {
    if(j == 6) { continue; }
    print(j);
}
println(' ')