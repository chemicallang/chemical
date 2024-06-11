// Copyright (c) Qinetik 2024.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "libtcc.h"

void handle_error(void *opaque, const char *msg)
{
    std::cout << "error happened : " << msg << std::endl;
}

/* this function is called by the generated code */
int add(int a, int b)
{
    return a + b;
}

/* this strinc is referenced by the generated code */
const char hello[] = "Hello World!";

char my_program[] =
        "#include <tcclib.h>\n" /* include the "Simple libc header for TCC" */
        "extern int add(int a, int b);\n"
        "#ifdef _WIN32\n" /* dynamically linked data needs 'dllimport' */
        " __attribute__((dllimport))\n"
        "#endif\n"
        "extern const char hello[];\n"
        "int fib(int n)\n"
        "{\n"
        "    if (n <= 2)\n"
        "        return 1;\n"
        "    else\n"
        "        return fib(n-1) + fib(n-2);\n"
        "}\n"
        "\n"
        "int foo(int n)\n"
        "{\n"
        "    printf(\"%s\\n\", hello);\n"
        "    printf(\"fib(%d) = %d\\n\", n, fib(n));\n"
        "    printf(\"add(%d, %d) = %d\\n\", n, 2 * n, add(n, 2 * n));\n"
        "    return 0;\n"
        "}\n";

int libtcc_test() {
    TCCState *s;
    int i;
    int (*func)(int);

    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create tcc state\n");
        exit(1);
    }

    /* set custom error/warning printer */
    tcc_set_error_func(s, stderr, handle_error);

    int result;

    result = tcc_add_include_path(s, "packages/tcc/include");;

    result = tcc_add_library_path(s, "packages/tcc/lib");

    /* MUST BE CALLED before any compilation */
    result = tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    if(result == -1) {
        std::cerr << "Couldn't set TinyCC output type" << std::endl;
        return 1;
    }

    if (tcc_compile_string(s, my_program) == -1) {
        std::cerr << "Couldn't compile the program" << std::endl;
        return 1;
    }

    /* as a test, we add symbols that the compiled program can use.
       You may also open a dll with tcc_add_dll() and use symbols from that */
    tcc_add_symbol(s, "add", (const void*) add);
    tcc_add_symbol(s, "hello", hello);

    /* relocate the code */
    if (tcc_relocate(s) < 0)
        return 1;

    /* get entry symbol */
    func = (int (*)(int)) tcc_get_symbol(s, "foo");
    if (!func)
        return 1;

    /* run the code */
    func(32);

    /* delete the state */
    tcc_delete(s);

    return 0;

}