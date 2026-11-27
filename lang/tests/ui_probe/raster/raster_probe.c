/*
 * P0.1 raster go/no-go prototype (throwaway, not shipped).
 *
 * Scope: prove the CPU raster budgets in lang/docs/ui/BENCHMARKS.md §2.2/§2.3
 * are reachable on the actual host before P1.9. Premultiplied RGBA8,
 * sRGB-encoded 8-bit UNORM blending with integer div255 (no float in the inner
 * loop), analytic axis-aligned coverage AA (primary) and a 4x4 supersampled
 * variant (comparison), and an alpha-glyph blit.
 *
 * This is deliberately scalar C. If a scalar prototype cannot hit a budget,
 * the budget needs a documented correction before P1.9.
 *
 * Build:  cc -O2 -std=c11 -o raster_probe raster_probe.c
 * Run:    ./raster_probe [iterations]
 */
#define _POSIX_C_SOURCE 199309L
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif

#define W 1920
#define H 1080
#define PIX (W * H)

typedef struct {
    uint32_t state;
} Rng;

static Rng rng_make(uint32_t seed) {
    Rng r = {seed ? seed : 1u};
    return r;
}

static uint32_t rng_next(Rng *r) {
    uint32_t x = r->state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->state = x;
    return x;
}

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static int cmp_double(const void *a, const void *b) {
    double x = *(const double *)a;
    double y = *(const double *)b;
    return (x > y) - (x < y);
}

/* sRGB-encoded 8-bit UNORM: round(c / 255), integer only. */
static inline uint8_t div255(uint32_t c) {
    return (uint8_t)((c + 128u + ((c + 128u) >> 8)) >> 8);
}

static inline void blend_px(uint8_t *d, uint8_t sr, uint8_t sg, uint8_t sb,
                            uint8_t sa) {
    /* source and destination are premultiplied RGBA8 */
    uint32_t ia = 255u - sa;
    d[0] = (uint8_t)(sr + div255((uint32_t)d[0] * ia));
    d[1] = (uint8_t)(sg + div255((uint32_t)d[1] * ia));
    d[2] = (uint8_t)(sb + div255((uint32_t)d[2] * ia));
    d[3] = (uint8_t)(sa + div255((uint32_t)d[3] * ia));
}

static uint64_t g_sink;

static void clear_buffer(uint8_t *p, uint8_t v) {
    memset(p, v, (size_t)PIX * 4u);
    g_sink += p[0] + p[PIX * 4u - 1u] + p[(PIX / 2u) * 4u];
}

/* Opaque fill via 64-bit stores (what a real clear-to-color path does). */
static void fill_solid(uint8_t *p, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t px = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) |
                  ((uint32_t)a << 24);
    uint64_t two = (uint64_t)px | ((uint64_t)px << 32);
    uint64_t *q = (uint64_t *)p;
    size_t n = (size_t)PIX / 2u;
    for (size_t i = 0; i < n; i++) q[i] = two;
    g_sink += q[0] + q[n - 1u];
}

/* Solid, non-AA quad (the fast path a solid rect uses). */
static void fill_quad_solid(uint8_t *fb, int x0, int y0, int x1, int y1,
                            uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > W) x1 = W;
    if (y1 > H) y1 = H;
    for (int y = y0; y < y1; y++) {
        uint8_t *row = fb + ((size_t)y * W + (size_t)x0) * 4u;
        int n = x1 - x0;
        if (a == 255) {
            uint32_t px = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) |
                          ((uint32_t)a << 24);
            for (int i = 0; i < n; i++) ((uint32_t *)row)[i] = px;
        } else {
            for (int i = 0; i < n; i++) blend_px(row + (size_t)i * 4u, r, g, b, a);
        }
    }
    g_sink += fb[0];
}

/*
 * Analytic coverage for an axis-aligned rect. Returns coverage in 0..255.
 * The rect is (x0,y0)-(x1,y1) in device pixels; pixel (px,py) covers
 * [px,px+1) x [py,py+1). This is the exact area overlap, not 16 samples.
 */
static inline uint32_t aa_coverage(int px, int py, double x0, double y0,
                                   double x1, double y1) {
    double cx0 = (double)px > x0 ? (double)px : x0;
    double cy0 = (double)py > y0 ? (double)py : y0;
    double cx1 = (double)(px + 1) < x1 ? (double)(px + 1) : x1;
    double cy1 = (double)(py + 1) < y1 ? (double)(py + 1) : y1;
    double w = cx1 - cx0;
    double h = cy1 - cy0;
    if (w <= 0.0 || h <= 0.0) return 0u;
    double cov = w * h;
    if (cov >= 1.0) return 255u;
    return (uint32_t)(cov * 255.0 + 0.5);
}

static void aa_quad(uint8_t *fb, double x0, double y0, double x1, double y1,
                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int ix0 = (int)x0;
    int iy0 = (int)y0;
    int ix1 = (int)x1 + 1;
    int iy1 = (int)y1 + 1;
    if (ix0 < 0) ix0 = 0;
    if (iy0 < 0) iy0 = 0;
    if (ix1 > W) ix1 = W;
    if (iy1 > H) iy1 = H;
    /* Fully covered interior span; only edges pay for coverage math. */
    int sx0 = (int)(x0 + 0.999999);
    int sy0 = (int)(y0 + 0.999999);
    int sx1 = (int)x1;
    int sy1 = (int)y1;
    if (sx0 < ix0) sx0 = ix0;
    if (sy0 < iy0) sy0 = iy0;
    if (sx1 > ix1) sx1 = ix1;
    if (sy1 > iy1) sy1 = iy1;
    for (int y = iy0; y < iy1; y++) {
        uint8_t *row = fb + (size_t)y * W * 4u;
        int solid_row = (y >= sy0 && y < sy1);
        for (int x = ix0; x < ix1; x++) {
            uint32_t cov;
            if (solid_row && x >= sx0 && x < sx1) {
                cov = 255u;
            } else {
                cov = aa_coverage(x, y, x0, y0, x1, y1);
            }
            if (cov == 0u) continue;
            if (cov == 255u) {
                blend_px(row + (size_t)x * 4u, r, g, b, a);
            } else {
                uint32_t sa = ((uint32_t)a * cov) / 255u;
                uint32_t sr = ((uint32_t)r * cov) / 255u;
                uint32_t sg = ((uint32_t)g * cov) / 255u;
                uint32_t sb = ((uint32_t)b * cov) / 255u;
                blend_px(row + (size_t)x * 4u, (uint8_t)sr, (uint8_t)sg,
                         (uint8_t)sb, (uint8_t)sa);
            }
        }
    }
    g_sink += fb[0] + fb[((size_t)PIX - 1u) * 4u];
}

/* SSE2 solid-alpha span blend: dst = src + dst * (255-a). 4 pixels/iter. */
static void blend_span_sse2(uint8_t *p, int n, uint8_t r, uint8_t g, uint8_t b,
                            uint8_t a) {
#ifdef __SSE2__
    uint32_t ia = 255u - a;
    __m128i zero = _mm_setzero_si128();
    __m128i iav = _mm_set1_epi16((short)ia);
    __m128i c128 = _mm_set1_epi16(128);
    int i = 0;
    for (; i + 4 <= n; i += 4) {
        __m128i d = _mm_loadu_si128((const __m128i *)(p + (size_t)i * 4u));
        __m128i lo = _mm_unpacklo_epi8(d, zero);
        __m128i hi = _mm_unpackhi_epi8(d, zero);
        lo = _mm_mullo_epi16(lo, iav);
        hi = _mm_mullo_epi16(hi, iav);
        lo = _mm_add_epi16(lo, c128);
        hi = _mm_add_epi16(hi, c128);
        lo = _mm_add_epi16(lo, _mm_srli_epi16(lo, 8));
        hi = _mm_add_epi16(hi, _mm_srli_epi16(hi, 8));
        lo = _mm_srli_epi16(lo, 8);
        hi = _mm_srli_epi16(hi, 8);
        /* lo/hi lanes are (r,g,b,a) per pixel in order; add the premultiplied
         * source colour to each lane. */
        lo = _mm_add_epi16(lo, _mm_setr_epi16((short)r, (short)g, (short)b,
                                              (short)a, (short)r, (short)g,
                                              (short)b, (short)a));
        hi = _mm_add_epi16(hi, _mm_setr_epi16((short)r, (short)g, (short)b,
                                              (short)a, (short)r, (short)g,
                                              (short)b, (short)a));
        _mm_storeu_si128((__m128i *)(p + (size_t)i * 4u),
                         _mm_packus_epi16(lo, hi));
    }
    for (; i < n; i++) blend_px(p + (size_t)i * 4u, r, g, b, a);
#else
    for (int i = 0; i < n; i++) blend_px(p + (size_t)i * 4u, r, g, b, a);
#endif
}

/* Analytic AA quad whose fully covered interior rows use the SSE2 span. */
static void aa_quad_simd(uint8_t *fb, double x0, double y0, double x1,
                         double y1, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int ix0 = (int)x0;
    int iy0 = (int)y0;
    int ix1 = (int)x1 + 1;
    int iy1 = (int)y1 + 1;
    if (ix0 < 0) ix0 = 0;
    if (iy0 < 0) iy0 = 0;
    if (ix1 > W) ix1 = W;
    if (iy1 > H) iy1 = H;
    int sx0 = (int)(x0 + 0.999999);
    int sy0 = (int)(y0 + 0.999999);
    int sx1 = (int)x1;
    int sy1 = (int)y1;
    if (sx0 < ix0) sx0 = ix0;
    if (sy0 < iy0) sy0 = iy0;
    if (sx1 > ix1) sx1 = ix1;
    if (sy1 > iy1) sy1 = iy1;
    for (int y = iy0; y < iy1; y++) {
        uint8_t *row = fb + (size_t)y * W * 4u;
        int solid_row = (y >= sy0 && y < sy1);
        if (solid_row && sx1 > sx0) {
            for (int x = ix0; x < sx0; x++) {
                uint32_t cov = aa_coverage(x, y, x0, y0, x1, y1);
                if (cov == 0u) continue;
                uint32_t sa = ((uint32_t)a * cov) / 255u;
                blend_px(row + (size_t)x * 4u, (uint8_t)(((uint32_t)r * cov) / 255u),
                         (uint8_t)(((uint32_t)g * cov) / 255u),
                         (uint8_t)(((uint32_t)b * cov) / 255u), (uint8_t)sa);
            }
            blend_span_sse2(row + (size_t)sx0 * 4u, sx1 - sx0, r, g, b, a);
            for (int x = sx1; x < ix1; x++) {
                uint32_t cov = aa_coverage(x, y, x0, y0, x1, y1);
                if (cov == 0u) continue;
                uint32_t sa = ((uint32_t)a * cov) / 255u;
                blend_px(row + (size_t)x * 4u, (uint8_t)(((uint32_t)r * cov) / 255u),
                         (uint8_t)(((uint32_t)g * cov) / 255u),
                         (uint8_t)(((uint32_t)b * cov) / 255u), (uint8_t)sa);
            }
            continue;
        }
        for (int x = ix0; x < ix1; x++) {
            uint32_t cov = aa_coverage(x, y, x0, y0, x1, y1);
            if (cov == 0u) continue;
            uint32_t sa = ((uint32_t)a * cov) / 255u;
            blend_px(row + (size_t)x * 4u, (uint8_t)(((uint32_t)r * cov) / 255u),
                     (uint8_t)(((uint32_t)g * cov) / 255u),
                     (uint8_t)(((uint32_t)b * cov) / 255u), (uint8_t)sa);
        }
    }
    g_sink += fb[0] + fb[((size_t)PIX - 1u) * 4u];
}

/* Opaque AA quad: fully covered pixels are a 32-bit store. */
static void aa_quad_opaque(uint8_t *fb, double x0, double y0, double x1,
                           double y1, uint8_t r, uint8_t g, uint8_t b) {
    int ix0 = (int)x0;
    int iy0 = (int)y0;
    int ix1 = (int)x1 + 1;
    int iy1 = (int)y1 + 1;
    if (ix0 < 0) ix0 = 0;
    if (iy0 < 0) iy0 = 0;
    if (ix1 > W) ix1 = W;
    if (iy1 > H) iy1 = H;
    int sx0 = (int)(x0 + 0.999999);
    int sy0 = (int)(y0 + 0.999999);
    int sx1 = (int)x1;
    int sy1 = (int)y1;
    if (sx0 < ix0) sx0 = ix0;
    if (sy0 < iy0) sy0 = iy0;
    if (sx1 > ix1) sx1 = ix1;
    if (sy1 > iy1) sy1 = iy1;
    uint32_t px = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) |
                  0xff000000u;
    for (int y = iy0; y < iy1; y++) {
        uint8_t *row = fb + (size_t)y * W * 4u;
        int solid_row = (y >= sy0 && y < sy1);
        if (solid_row && sx1 > sx0) {
            uint32_t *p32 = (uint32_t *)(row + (size_t)sx0 * 4u);
            for (int x = sx0; x < sx1; x++) *p32++ = px;
        }
        for (int x = ix0; x < ix1; x++) {
            if (solid_row && x >= sx0 && x < sx1) continue;
            uint32_t cov = aa_coverage(x, y, x0, y0, x1, y1);
            if (cov == 0u) continue;
            blend_px(row + (size_t)x * 4u, (uint8_t)(((uint32_t)r * cov) / 255u),
                     (uint8_t)(((uint32_t)g * cov) / 255u),
                     (uint8_t)(((uint32_t)b * cov) / 255u), (uint8_t)cov);
        }
    }
    g_sink += fb[0] + fb[((size_t)PIX - 1u) * 4u];
}

/* 4x4 supersampled AA, for comparison with the analytic path. */
static void aa_quad_4x4(uint8_t *fb, double x0, double y0, double x1, double y1,
                        uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    int ix0 = (int)x0;
    int iy0 = (int)y0;
    int ix1 = (int)x1 + 1;
    int iy1 = (int)y1 + 1;
    if (ix0 < 0) ix0 = 0;
    if (iy0 < 0) iy0 = 0;
    if (ix1 > W) ix1 = W;
    if (iy1 > H) iy1 = H;
    static const double off[4] = {0.125, 0.375, 0.625, 0.875};
    for (int y = iy0; y < iy1; y++) {
        uint8_t *row = fb + (size_t)y * W * 4u;
        for (int x = ix0; x < ix1; x++) {
            int hits = 0;
            for (int sy = 0; sy < 4; sy++) {
                double py = (double)y + off[sy];
                for (int sx = 0; sx < 4; sx++) {
                    double px = (double)x + off[sx];
                    if (px >= x0 && px < x1 && py >= y0 && py < y1) hits++;
                }
            }
            if (hits == 0) continue;
            uint32_t cov = (uint32_t)hits * 255u / 16u;
            if (cov == 255u) {
                blend_px(row + (size_t)x * 4u, r, g, b, a);
            } else {
                uint32_t sa = ((uint32_t)a * cov) / 255u;
                uint32_t sr = ((uint32_t)r * cov) / 255u;
                uint32_t sg = ((uint32_t)g * cov) / 255u;
                uint32_t sb = ((uint32_t)b * cov) / 255u;
                blend_px(row + (size_t)x * 4u, (uint8_t)sr, (uint8_t)sg,
                         (uint8_t)sb, (uint8_t)sa);
            }
        }
    }
}

static void glyph_blit(uint8_t *fb, int x, int y, const uint8_t *alpha,
                       int gw, int gh, uint8_t r, uint8_t g, uint8_t b) {
    for (int gy = 0; gy < gh; gy++) {
        int py = y + gy;
        if (py < 0 || py >= H) continue;
        uint8_t *row = fb + (size_t)py * W * 4u;
        const uint8_t *arow = alpha + (size_t)gy * (size_t)gw;
        for (int gx = 0; gx < gw; gx++) {
            int px = x + gx;
            if (px < 0 || px >= W) continue;
            uint32_t a8 = arow[gx];
            if (a8 == 0u) continue;
            uint8_t *d = row + (size_t)px * 4u;
            blend_px(d, (uint8_t)(((uint32_t)r * a8) / 255u),
                     (uint8_t)(((uint32_t)g * a8) / 255u),
                     (uint8_t)(((uint32_t)b * a8) / 255u), (uint8_t)a8);
        }
    }
    g_sink += fb[0];
}

/* Copy one damaged rect (row by row, exactly what a present path does). */
static void damage_blit(const uint8_t *src, uint8_t *dst, int x, int y, int w,
                        int h) {
    for (int row = 0; row < h; row++) {
        memcpy(dst + ((size_t)(y + row) * W + (size_t)x) * 4u,
               src + ((size_t)(y + row) * W + (size_t)x) * 4u,
               (size_t)w * 4u);
    }
    g_sink += dst[0] + dst[((size_t)H - 1u) * W * 4u];
}

static void report_pss(void) {
    FILE *f = fopen("/proc/self/status", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "VmHWM:", 6) == 0) {
            printf("%s", line);
        }
    }
    fclose(f);
}

static void stats(const char *name, const char *detail, double *samples,
                  int n, double px) {
    qsort(samples, (size_t)n, sizeof(double), cmp_double);
    double med = samples[n / 2];
    double min = samples[0];
    double p99 = samples[n - 1];
    printf("%-28s %-30s min=%8.3f ms  med=%8.3f ms  max=%8.3f ms",
           name, detail, min, med, p99);
    if (px > 0.0) {
        printf("  %.2f Mpx/s", (px / med) / 1000.0);
    }
    printf("\n");
}

int main(int argc, char **argv) {
    int iters = argc > 1 ? atoi(argv[1]) : 7;
    if (iters < 1) iters = 1;
    if (iters > 100) iters = 100;

    uint8_t *back = (uint8_t *)malloc((size_t)PIX * 4u);
    uint8_t *front = (uint8_t *)malloc((size_t)PIX * 4u);
    if (!back || !front) {
        fprintf(stderr, "raster_probe: out of memory\n");
        return 2;
    }
    clear_buffer(back, 0);
    clear_buffer(front, 0);

    /* Glyph atlas: 256 glyphs of 16x16 alpha, deterministic stamps. */
    enum { GW = 16, GH = 16, GCOUNT = 256 };
    uint8_t *atlas = (uint8_t *)malloc((size_t)GW * GH * GCOUNT);
    if (!atlas) return 2;
    Rng gr = rng_make(0xC0FFEEu);
    for (int g = 0; g < GCOUNT; g++) {
        for (int y = 0; y < GH; y++) {
            for (int x = 0; x < GW; x++) {
                uint32_t v = rng_next(&gr);
                atlas[((size_t)g * GH + (size_t)y) * GW + (size_t)x] =
                    (uint8_t)((v % 5u) == 0u ? (v >> 8) : 0u);
            }
        }
    }

    double *s = (double *)malloc((size_t)iters * sizeof(double));
    if (!s) return 2;

    printf("P0.1 raster prototype: %dx%d RGBA8, %d iterations, scalar C -O2\n",
           W, H, iters);
    report_pss();

    /* 1. 1080p clear (transparent) and opaque fill. */
    for (int i = 0; i < iters; i++) {
        double t = now_ms();
        clear_buffer(back, 0);
        s[i] = now_ms() - t;
    }
    stats("clear_1080p", "memset 8.3 MB transparent", s, iters, (double)PIX);

    for (int i = 0; i < iters; i++) {
        double t = now_ms();
        fill_solid(back, 24, 26, 32, 255);
        s[i] = now_ms() - t;
    }
    stats("fill_1080p_opaque", "solid color 8.3 MB", s, iters, (double)PIX);

    /* 2. 1k and 10k defined AA quads (dashboard-like small tiles). */
    /* 100x100 grid over 1920x1080: each cell ~19.2x10.8, inset 1.5px. */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                double x = (double)gx * 19.2 + 1.5;
                double y = (double)gy * 10.8 + 1.5;
                aa_quad(back, x, y, x + 16.2, y + 7.8, 200, 80, 40, 180);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_10k_small", "100x100 grid, ~16x8 px each", s, iters,
          10000.0 * 16.2 * 7.8);

    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 1000; gy++) {
            for (int gx = 0; gx < 100 && n < 1000; gx++) {
                double x = (double)gx * 19.2 + 1.5;
                double y = (double)gy * 10.8 + 1.5;
                aa_quad(back, x, y, x + 16.2, y + 7.8, 200, 80, 40, 180);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_1k_small", "100x10 grid, ~16x8 px each", s, iters,
          1000.0 * 16.2 * 7.8);

    /* 3. 10k medium 48x24 quads (cards/buttons). */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                double x = (double)gx * 19.2;
                double y = (double)gy * 10.8;
                aa_quad(back, x, y, x + 48.0, y + 24.0, 60, 120, 200, 160);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_10k_medium", "10k x 48x24 (overlapping)", s, iters,
          10000.0 * 48.0 * 24.0);

    /* 3b. 4x4 supersampled variant on the small grid, for comparison. */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                double x = (double)gx * 19.2 + 1.5;
                double y = (double)gy * 10.8 + 1.5;
                aa_quad_4x4(back, x, y, x + 16.2, y + 7.8, 200, 80, 40, 180);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_10k_4x4", "same grid, 16 samples/pixel", s, iters,
          10000.0 * 16.2 * 7.8);

    /* 3c. Same scene, interior spans blended with SSE2. */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                double x = (double)gx * 19.2 + 1.5;
                double y = (double)gy * 10.8 + 1.5;
                aa_quad_simd(back, x, y, x + 16.2, y + 7.8, 200, 80, 40, 180);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_10k_simd", "same grid, SSE2 interior span", s, iters,
          10000.0 * 16.2 * 7.8);

    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                double x = (double)gx * 19.2;
                double y = (double)gy * 10.8;
                aa_quad_simd(back, x, y, x + 48.0, y + 24.0, 60, 120, 200, 160);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_10k_medium_simd", "10k x 48x24, SSE2 interior", s, iters,
          10000.0 * 48.0 * 24.0);

    /* 3d. Opaque AA quads: interior is a 32-bit store. */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                double x = (double)gx * 19.2 + 1.5;
                double y = (double)gy * 10.8 + 1.5;
                aa_quad_opaque(back, x, y, x + 16.2, y + 7.8, 200, 80, 40);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("aa_quads_10k_opaque", "same grid, opaque 32-bit interior", s,
          iters, 10000.0 * 16.2 * 7.8);

    /* 4. 5k and 10k cached glyph blits (16x16 stamp). */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        Rng r = rng_make(0x1234567u);
        double t = now_ms();
        for (int n = 0; n < 5000; n++) {
            uint32_t v = rng_next(&r);
            int gx = (int)(v % W);
            int gy = (int)((v >> 10) % H);
            int g = (int)((v >> 20) % GCOUNT);
            glyph_blit(back, gx, gy, atlas + (size_t)g * GW * GH, GW, GH, 240,
                       240, 240);
        }
        s[i] = now_ms() - t;
    }
    stats("glyph_blits_5k", "16x16 alpha stamps", s, iters, 5000.0 * 256.0);

    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        Rng r = rng_make(0x1234567u);
        double t = now_ms();
        for (int n = 0; n < 10000; n++) {
            uint32_t v = rng_next(&r);
            int gx = (int)(v % W);
            int gy = (int)((v >> 10) % H);
            int g = (int)((v >> 20) % GCOUNT);
            glyph_blit(back, gx, gy, atlas + (size_t)g * GW * GH, GW, GH, 240,
                       240, 240);
        }
        s[i] = now_ms() - t;
    }
    stats("glyph_blits_10k", "16x16 alpha stamps", s, iters, 10000.0 * 256.0);

    /* 4b. Sequential (text-like) glyph blits, cache-friendly order. */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        for (int n = 0; n < 5000; n++) {
            int col = n % 200;
            int line = (n / 200) % 50;
            int gx = 20 + col * 9;
            int gy = 20 + line * 20;
            int g = n % GCOUNT;
            glyph_blit(back, gx, gy, atlas + (size_t)g * GW * GH, GW, GH, 240,
                       240, 240);
        }
        s[i] = now_ms() - t;
    }
    stats("glyph_blits_5k_seq", "text-order 16x16 stamps", s, iters, 5000.0 * 256.0);

    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        for (int n = 0; n < 10000; n++) {
            int col = n % 200;
            int line = (n / 200) % 50;
            int gx = 20 + col * 9;
            int gy = 20 + line * 20;
            int g = n % GCOUNT;
            glyph_blit(back, gx, gy, atlas + (size_t)g * GW * GH, GW, GH, 240,
                       240, 240);
        }
        s[i] = now_ms() - t;
    }
    stats("glyph_blits_10k_seq", "text-order 16x16 stamps", s, iters, 10000.0 * 256.0);

    /* 5. Damage blit: full frame and a 200x120 dirty rect. */
    for (int i = 0; i < iters; i++) {
        double t = now_ms();
        damage_blit(back, front, 0, 0, W, H);
        s[i] = now_ms() - t;
    }
    stats("damage_blit_full", "1080p frame copy", s, iters, (double)PIX);

    for (int i = 0; i < iters; i++) {
        double t = now_ms();
        damage_blit(back, front, 300, 200, 200, 120);
        s[i] = now_ms() - t;
    }
    stats("damage_blit_200x120", "24k pixels", s, iters, 200.0 * 120.0);

    /* 6. 10k solid quads (the BENCHMARKS §2.3 paint row). */
    for (int i = 0; i < iters; i++) {
        clear_buffer(back, 0);
        double t = now_ms();
        int n = 0;
        for (int gy = 0; gy < 100 && n < 10000; gy++) {
            for (int gx = 0; gx < 100 && n < 10000; gx++) {
                fill_quad_solid(back, (int)(gx * 19.2) + 1,
                                (int)(gy * 10.8) + 1,
                                (int)(gx * 19.2) + 16,
                                (int)(gy * 10.8) + 8, 200, 80, 40, 180);
                n++;
            }
        }
        s[i] = now_ms() - t;
    }
    stats("solid_quads_10k", "~15x7 px each", s, iters, 10000.0 * 15.0 * 7.0);

    report_pss();
    free(s);
    free(atlas);
    free(back);
    free(front);
    return 0;
}
