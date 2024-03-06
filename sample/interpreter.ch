var x : int = 1;
if(x == 1) {
    x += 8 + 2;
}
println(x);
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