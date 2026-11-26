/*
 * Freestanding wasm32 runtime for Chemical.
 *
 * This file is compiled with `clang --target=wasm32-unknown-unknown -nostdlib`
 * and linked into every wasm module (it is `link c ... if wasm32` in cstd).
 * It provides only what the wasm-safe std subset needs:
 *
 *   - a first-fit allocator with a free list over `__heap_base` + memory.grow
 *   - memory and string primitives
 *   - abort/trap and a minimal printf family
 *   - fwrite/fflush routed to the host `env.chc_log(ptr, len)` import
 *
 * There are deliberately no files, sockets, processes, threads, clocks, locale,
 * or host libc calls. Anything outside this surface is not part of the wasm
 * profile (see lang/docs/wasm-target.md).
 */

typedef __SIZE_TYPE__ size_t;
typedef __UINT32_TYPE__ u32;
typedef __INT32_TYPE__ i32;
typedef __UINT64_TYPE__ u64;
typedef __INT64_TYPE__ i64;
typedef __UINTPTR_TYPE__ uintptr_t;

typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(dst, src) __builtin_va_copy(dst, src)

#define NULL ((void *)0)

/* Provided by wasm-ld. */
extern unsigned char __heap_base;

/* Host import (JS side: `env.chc_log`). Declared weak so an unhosted module
 * only fails if it actually writes to the console: wasm-ld leaves the import
 * undefined, and the JS host installs it. */
extern void chc_log(const void *ptr, size_t len);

/* ------------------------------------------------------------------------- */
/* memory primitives                                                         */
/* ------------------------------------------------------------------------- */

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    /* word-at-a-time when both sides agree on alignment */
    while (n >= 4 && (((uintptr_t)d | (uintptr_t)s) & 3u) == 0) {
        *(u32 *)d = *(const u32 *)s;
        d += 4;
        s += 4;
        n -= 4;
    }
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) return dst;
    if (d < s) {
        return memcpy(dst, src, n);
    }
    d += n;
    s += n;
    while (n--) {
        *--d = *--s;
    }
    return dst;
}

void *memset(void *dst, int value, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char v = (unsigned char)value;
    while (n >= 4 && ((uintptr_t)d & 3u) == 0) {
        *(u32 *)d = (u32)v * 0x01010101u;
        d += 4;
        n -= 4;
    }
    while (n--) {
        *d++ = v;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    while (n--) {
        if (*x != *y) return (int)*x - (int)*y;
        x++;
        y++;
    }
    return 0;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (*p) p++;
    return (size_t)(p - s);
}

int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    while (n && *a && *a == *b) {
        a++;
        b++;
        n--;
    }
    if (n == 0) return 0;
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *d = dst;
    while (n && *src) {
        *d++ = *src++;
        n--;
    }
    while (n--) {
        *d++ = 0;
    }
    return dst;
}

char *strcpy(char *dst, const char *src) {
    char *d = dst;
    while ((*d++ = *src++)) {
    }
    return dst;
}

/* ------------------------------------------------------------------------- */
/* allocator                                                                 */
/* ------------------------------------------------------------------------- */

typedef struct Block {
    size_t size;        /* payload bytes */
    struct Block *next; /* free-list link, sorted by address */
    u32 free;
    u32 pad;
} Block;

#define ALIGN 16u
#define BLOCK_HEADER ((size_t)sizeof(Block))
#define PAGE_SIZE 65536u

static Block *free_list = NULL;
static uintptr_t heap_top = 0;

static size_t align_up(size_t n) {
    return (n + (ALIGN - 1u)) & ~(size_t)(ALIGN - 1u);
}

static void heap_init(void) {
    if (heap_top == 0) {
        heap_top = align_up((uintptr_t)&__heap_base);
    }
}

static int heap_reserve(uintptr_t end) {
    const size_t current = (size_t)__builtin_wasm_memory_size(0) * PAGE_SIZE;
    if (end <= current) return 1;
    const size_t need = end - current;
    const size_t pages = (need + PAGE_SIZE - 1u) / PAGE_SIZE;
    if (__builtin_wasm_memory_grow(0, pages) == (size_t)-1) return 0;
    return 1;
}

void *malloc(size_t n) {
    heap_init();
    if (n == 0) n = 1;
    const size_t need = align_up(n);

    Block *prev = NULL;
    Block *cur = free_list;
    while (cur) {
        if (cur->free && cur->size >= need) {
            /* split when the tail can hold a header plus one aligned unit */
            if (cur->size >= need + BLOCK_HEADER + ALIGN) {
                Block *tail = (Block *)((unsigned char *)(cur + 1) + need);
                tail->size = cur->size - need - BLOCK_HEADER;
                tail->free = 1;
                tail->pad = 0;
                tail->next = cur->next;
                cur->size = need;
                cur->next = tail;
            }
            cur->free = 0;
            return (void *)(cur + 1);
        }
        prev = cur;
        cur = cur->next;
    }
    (void)prev;

    const uintptr_t start = heap_top;
    const uintptr_t end = start + BLOCK_HEADER + need;
    if (!heap_reserve(end)) return NULL;
    Block *b = (Block *)start;
    b->size = need;
    b->next = NULL;
    b->free = 0;
    b->pad = 0;
    heap_top = end;
    return (void *)(b + 1);
}

void free(void *p) {
    if (p == NULL) return;
    Block *b = ((Block *)p) - 1;
    if (b->free) return; /* double free: ignore rather than corrupt the list */
    b->free = 1;

    /* insert sorted by address */
    Block *prev = NULL;
    Block *cur = free_list;
    while (cur && cur < b) {
        prev = cur;
        cur = cur->next;
    }
    b->next = cur;
    if (prev) {
        prev->next = b;
    } else {
        free_list = b;
    }

    /* coalesce with the following block */
    if (b->next) {
        Block *next = b->next;
        if ((unsigned char *)(b + 1) + b->size == (unsigned char *)next) {
            b->size += BLOCK_HEADER + next->size;
            b->next = next->next;
        }
    }
    /* coalesce with the preceding block */
    if (prev) {
        if ((unsigned char *)(prev + 1) + prev->size == (unsigned char *)b) {
            prev->size += BLOCK_HEADER + b->size;
            prev->next = b->next;
        }
    }
}

void *realloc(void *p, size_t n) {
    if (p == NULL) return malloc(n);
    if (n == 0) {
        free(p);
        return NULL;
    }
    Block *b = ((Block *)p) - 1;
    if (b->size >= n) return p;
    void *np = malloc(n);
    if (np == NULL) return NULL;
    memcpy(np, p, b->size);
    free(p);
    return np;
}

void *calloc(size_t count, size_t size) {
    if (count != 0 && size > (size_t)-1 / count) return NULL;
    const size_t total = count * size;
    void *p = malloc(total);
    if (p != NULL) memset(p, 0, total);
    return p;
}

/* ------------------------------------------------------------------------- */
/* console / panic                                                           */
/* ------------------------------------------------------------------------- */

typedef struct {
    char *buf;
    size_t cap; /* including the terminating NUL, 0 = count only */
    size_t len;
} Sink;

static void sink_put(Sink *s, char c) {
    if (s->cap && s->len + 1 < s->cap) {
        s->buf[s->len] = c;
    }
    s->len++;
}

static void sink_pad(Sink *s, char c, int count) {
    while (count-- > 0) sink_put(s, c);
}

static void sink_unsigned(Sink *s, u64 value, unsigned base, int upper,
                          int negative, int width, char pad, int left) {
    char tmp[24];
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int n = 0;
    if (value == 0) {
        tmp[n++] = '0';
    } else {
        while (value) {
            tmp[n++] = digits[(unsigned)(value % base)];
            value /= base;
        }
    }
    const int total = n + (negative ? 1 : 0);
    if (!left && pad == ' ') {
        sink_pad(s, ' ', width - total);
    }
    if (negative) {
        sink_put(s, '-');
    }
    if (!left && pad == '0') {
        sink_pad(s, '0', width - total);
    }
    while (n--) {
        sink_put(s, tmp[n]);
    }
    if (left) {
        sink_pad(s, ' ', width - total);
    }
}

static size_t format_core(Sink *s, const char *fmt, va_list ap) {
    for (const char *p = fmt; *p; p++) {
        if (*p != '%') {
            sink_put(s, *p);
            continue;
        }
        p++;
        if (*p == '%') {
            sink_put(s, '%');
            continue;
        }
        int left = 0;
        char pad = ' ';
        int width = 0;
        while (*p == '-' || *p == '0' || *p == ' ') {
            if (*p == '-') left = 1;
            if (*p == '0') pad = '0';
            p++;
        }
        while (*p >= '0' && *p <= '9') {
            width = width * 10 + (*p - '0');
            p++;
        }
        int is_long = 0;
        while (*p == 'l' || *p == 'z' || *p == 'j') {
            is_long = 1;
            p++;
        }
        while (*p == 'h') {
            p++;
        }
        switch (*p) {
        case 'd':
        case 'i': {
            i64 v = is_long ? va_arg(ap, i64) : (i64)va_arg(ap, int);
            int neg = v < 0;
            u64 mag = neg ? (u64)(-(v + 1)) + 1u : (u64)v;
            sink_unsigned(s, mag, 10, 0, neg, width, pad, left);
            break;
        }
        case 'u':
        case 'x':
        case 'X': {
            u64 v = is_long ? va_arg(ap, u64) : (u64)va_arg(ap, unsigned);
            const unsigned base = (*p == 'u') ? 10u : 16u;
            sink_unsigned(s, v, base, *p == 'X', 0, width, pad, left);
            break;
        }
        case 'p': {
            u32 v = (u32)(uintptr_t)va_arg(ap, void *);
            sink_pad(s, ' ', width - 10);
            sink_put(s, '0');
            sink_put(s, 'x');
            sink_unsigned(s, v, 16, 0, 0, 8, '0', 0);
            break;
        }
        case 'c':
            sink_put(s, (char)va_arg(ap, int));
            break;
        case 's': {
            const char *str = va_arg(ap, const char *);
            if (!str) str = "(null)";
            const int n = (int)strlen(str);
            if (!left) sink_pad(s, ' ', width - n);
            for (const char *q = str; *q; q++) sink_put(s, *q);
            if (left) sink_pad(s, ' ', width - n);
            break;
        }
        case 0:
            p--;
            break;
        default:
            sink_put(s, '%');
            sink_put(s, *p);
            break;
        }
    }
    return s->len;
}

int vsnprintf(char *buf, size_t cap, const char *fmt, va_list ap) {
    Sink s;
    s.buf = buf;
    s.cap = cap;
    s.len = 0;
    const size_t n = format_core(&s, fmt, ap);
    if (cap) {
        const size_t end = n < cap - 1 ? n : cap - 1;
        buf[end] = 0;
    }
    return (int)n;
}

int snprintf(char *buf, size_t cap, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    const int n = vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    return n;
}

int printf(const char *fmt, ...) {
    static char buffer[2048];
    va_list ap;
    va_start(ap, fmt);
    Sink s;
    s.buf = buffer;
    s.cap = sizeof(buffer);
    s.len = 0;
    format_core(&s, fmt, ap);
    va_end(ap);
    size_t len = s.len < sizeof(buffer) ? s.len : sizeof(buffer);
    buffer[len < sizeof(buffer) ? len : sizeof(buffer) - 1] = 0;
    chc_log(buffer, len);
    return (int)len;
}

int fflush(void *stream) {
    (void)stream;
    return 0;
}

size_t fwrite(const void *ptr, size_t size, size_t count, void *stream) {
    (void)stream;
    if (ptr == NULL || size == 0 || count == 0) return 0;
    chc_log(ptr, size * count);
    return count;
}

int putchar(int c) {
    const char ch = (char)c;
    chc_log(&ch, 1);
    return c;
}

int puts(const char *str) {
    if (str == NULL) return -1;
    const size_t n = strlen(str);
    chc_log(str, n);
    const char nl = '\n';
    chc_log(&nl, 1);
    return 0;
}

void abort(void) {
    __builtin_trap();
}

void __chc_wasm_unreachable(void) {
    __builtin_trap();
}
