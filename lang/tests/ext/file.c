// Copyright (c) Chemical Language Foundation 2025.

#include <stdarg.h>

extern int check_external_sum(int a, int b) {
    return a + b;
}

// Reads the C-promoted varargs: float -> double, bool/i8/u8 -> int.
extern int check_vararg_promotions(int count, ...) {
    if (count != 4) return 0;
    va_list ap;
    va_start(ap, count);
    const double f = va_arg(ap, double);
    const int small = va_arg(ap, int);
    const int flag = va_arg(ap, int);
    const int byte = va_arg(ap, int);
    va_end(ap);
    return f == 2.5 && small == -7 && flag == 1 && byte == 200;
}