struct CAbiPair { int a; int b; };
struct CAbiBig { int a; int b; int c; int d; int e; };
struct CAbiFloats { float a; float b; };
struct CAbiMixed { int a; float b; };
struct CAbiThree { int a; int b; int c; };
union CAbiUnion { int i; float f; };

struct CAbiPair cabi_pair_add(struct CAbiPair p) { p.a += 1; p.b += 2; return p; }
int cabi_pair_sum(struct CAbiPair p) { return p.a + p.b; }
int cabi_pair_sum_extra(struct CAbiPair p, int extra) { return p.a + p.b + extra; }
struct CAbiBig cabi_big_add(struct CAbiBig x) {
    x.a += 1; x.b += 2; x.c += 3; x.d += 4; x.e += 5; return x;
}
struct CAbiFloats cabi_floats_add(struct CAbiFloats x) {
    x.a += 1.0f; x.b += 2.0f; return x;
}
struct CAbiMixed cabi_mixed_add(struct CAbiMixed x) {
    x.a += 1; x.b += 2.0f; return x;
}
struct CAbiThree cabi_three_add(struct CAbiThree x) {
    x.a += 1; x.b += 2; x.c += 3; return x;
}
int cabi_union_read(union CAbiUnion u) { return u.i; }
int cabi_two_pairs(struct CAbiPair a, struct CAbiPair b) { return a.a + b.b; }
