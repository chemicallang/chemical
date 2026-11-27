/*
 * P0.1 allocation probe shim (Linux/glibc only, throwaway).
 *
 * Counts process allocator calls and can fail the Nth allocation to prove the
 * std OOM behavior (abort, no unwinding). Build:
 *   cc -shared -fPIC -O2 -o alloc_shim.so alloc_shim.c -ldl
 * Run:
 *   UI_PROBE_COUNT=1 LD_PRELOAD=./alloc_shim.so ./app
 *   UI_PROBE_FAIL_AFTER=5 LD_PRELOAD=./alloc_shim.so ./app
 *
 * This is evidence for CONTRACTS §7.3: the UI runtime still needs an internal,
 * allocation-free counter; this only measures the process allocator.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void *(*real_malloc)(size_t);
static void *(*real_calloc)(size_t, size_t);
static void *(*real_realloc)(void *, size_t);
static void (*real_free)(void *);

static unsigned long g_malloc_calls;
static unsigned long g_calloc_calls;
static unsigned long g_realloc_calls;
static unsigned long g_free_calls;
static unsigned long g_limit;
static unsigned long g_alloc_seq;
static int g_initializing;

/* Tiny bump area so dlsym() itself can allocate during bootstrap. */
static char g_boot[65536];
static size_t g_boot_used;

static void *boot_alloc(size_t n) {
    size_t aligned = (n + 15u) & ~(size_t)15u;
    if (g_boot_used + aligned > sizeof(g_boot)) return NULL;
    void *p = &g_boot[g_boot_used];
    g_boot_used += aligned;
    return p;
}

static void init_once(void) {
    if (real_malloc) return;
    g_initializing = 1;
    real_malloc = (void *(*)(size_t))dlsym(RTLD_NEXT, "malloc");
    real_calloc = (void *(*)(size_t, size_t))dlsym(RTLD_NEXT, "calloc");
    real_realloc = (void *(*)(void *, size_t))dlsym(RTLD_NEXT, "realloc");
    real_free = (void (*)(void *))dlsym(RTLD_NEXT, "free");
    const char *env = getenv("UI_PROBE_FAIL_AFTER");
    if (env && *env) g_limit = strtoul(env, NULL, 10);
    g_initializing = 0;
}

static int should_fail(void) {
    g_alloc_seq++;
    return g_limit != 0 && g_alloc_seq == g_limit;
}

void *malloc(size_t n) {
    if (!real_malloc) {
        init_once();
        if (g_initializing && !real_malloc) return boot_alloc(n);
    }
    g_malloc_calls++;
    if (should_fail()) return NULL;
    return real_malloc(n);
}

void *calloc(size_t n, size_t sz) {
    if (!real_calloc) {
        init_once();
        if (g_initializing && !real_calloc) {
            void *p = boot_alloc(n * sz);
            if (p) memset(p, 0, n * sz);
            return p;
        }
    }
    g_calloc_calls++;
    if (should_fail()) return NULL;
    return real_calloc(n, sz);
}

void *realloc(void *p, size_t n) {
    if (!real_realloc) {
        init_once();
        if (g_initializing && !real_realloc) return NULL;
    }
    g_realloc_calls++;
    if (should_fail()) return NULL;
    return real_realloc(p, n);
}

void free(void *p) {
    if (!real_free) {
        init_once();
        return;
    }
    if (p >= (void *)g_boot && p < (void *)(g_boot + sizeof(g_boot))) return;
    g_free_calls++;
    real_free(p);
}

__attribute__((destructor)) static void report(void) {
    if (!getenv("UI_PROBE_COUNT")) return;
    char buf[256];
    int n = snprintf(buf, sizeof(buf),
                     "P0.1 alloc_shim malloc=%lu calloc=%lu realloc=%lu free=%lu "
                     "total=%lu\n",
                     g_malloc_calls, g_calloc_calls, g_realloc_calls,
                     g_free_calls,
                     g_malloc_calls + g_calloc_calls + g_realloc_calls);
    if (n > 0) {
        ssize_t ignored = write(2, buf, (size_t)n);
        (void)ignored;
    }
}
