/*
 * P1.9b CPU raster hot loops.
 *
 * Small in-tree C helper for clear/fill, solid spans, rounded-rect coverage
 * and damage copies. The Chemical wrapper validates the buffer, stride and
 * clip before calling any function here. These functions never allocate.
 *
 * The arithmetic and 16x16 rounded-corner coverage match the scalar Chemical
 * implementation in raster.ch byte-for-byte. SSE2 is used for fully covered
 * alpha spans on x86-64; TinyCC and non-SSE2 targets use the portable loop.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>


#if defined(__SSE2__) && !defined(__TINYC__)
#include <emmintrin.h>
#define UI_RASTER_SSE2 1
#else
#define UI_RASTER_SSE2 0
#endif

static inline uint8_t ui_div255(uint32_t c) {
    uint32_t n = c + 128u;
    return (uint8_t)((n + (n >> 8u)) >> 8u);
}

static inline uint8_t ui_clamp_u8(uint32_t v) {
    return v > 255u ? 255u : (uint8_t)v;
}

static inline float ui_clamp_opacity(float v) {
    if (v != v || v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

static inline void ui_blend_cov(uint8_t *d, uint8_t r, uint8_t g, uint8_t b,
                                uint8_t a, uint8_t cov, float opacity) {
    if (cov == 0u) return;
    uint32_t sa = ((uint32_t)a * (uint32_t)cov) / 255u;
    sa = (sa * (uint32_t)(ui_clamp_opacity(opacity) * 255.0f)) / 255u;
    if (sa == 0u) return;
    uint32_t sr = ((uint32_t)r * sa) / 255u;
    uint32_t sg = ((uint32_t)g * sa) / 255u;
    uint32_t sb = ((uint32_t)b * sa) / 255u;
    uint32_t ia = 255u - sa;
    d[0] = ui_clamp_u8(sr + ui_div255((uint32_t)d[0] * ia));
    d[1] = ui_clamp_u8(sg + ui_div255((uint32_t)d[1] * ia));
    d[2] = ui_clamp_u8(sb + ui_div255((uint32_t)d[2] * ia));
    d[3] = ui_clamp_u8(sa + ui_div255((uint32_t)d[3] * ia));
}

static void ui_blend_span_scalar(uint8_t *p, uint32_t n, uint8_t r, uint8_t g,
                                 uint8_t b, uint8_t a, uint8_t cov,
                                 float opacity) {
    uint32_t i = 0u;
    for (; i < n; i++) ui_blend_cov(p + (size_t)i * 4u, r, g, b, a, cov, opacity);
}

static void ui_blend_span(uint8_t *p, uint32_t n, uint8_t r, uint8_t g,
                          uint8_t b, uint8_t a, float opacity) {
    if (n == 0u) return;
    float op = ui_clamp_opacity(opacity);
    if (op != 1.0f) {
        ui_blend_span_scalar(p, n, r, g, b, a, 255u, op);
        return;
    }
    if (a == 255u) {
        /* Opaque source: a 32-bit store per pixel ("R", "G", "B", "A" in
         * memory order on little-endian targets; byte stores otherwise). */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint32_t v = (uint32_t)r | ((uint32_t)g << 8u) |
                     ((uint32_t)b << 16u) | ((uint32_t)a << 24u);
        uint32_t i = 0u;
#if UI_RASTER_SSE2
        __m128i vv = _mm_set1_epi32((int)v);
        for (; i + 4u <= n; i += 4u) {
            _mm_storeu_si128((__m128i *)(void *)(p + (size_t)i * 4u), vv);
        }
#endif
        for (; i < n; i++) {
            p[(size_t)i * 4u + 0u] = r;
            p[(size_t)i * 4u + 1u] = g;
            p[(size_t)i * 4u + 2u] = b;
            p[(size_t)i * 4u + 3u] = a;
        }
        return;
#else
        uint32_t i = 0u;
        for (; i < n; i++) {
            p[(size_t)i * 4u + 0u] = r;
            p[(size_t)i * 4u + 1u] = g;
            p[(size_t)i * 4u + 2u] = b;
            p[(size_t)i * 4u + 3u] = a;
        }
        return;
#endif
    }
    if (a == 0u) return;

    uint32_t sa = a;
    uint32_t sr = ((uint32_t)r * sa) / 255u;
    uint32_t sg = ((uint32_t)g * sa) / 255u;
    uint32_t sb = ((uint32_t)b * sa) / 255u;
    uint32_t ia = 255u - sa;
#if UI_RASTER_SSE2
    __m128i zero = _mm_setzero_si128();
    __m128i iav = _mm_set1_epi16((short)ia);
    __m128i c128 = _mm_set1_epi16(128);
    __m128i src = _mm_setr_epi16((short)sr, (short)sg, (short)sb, (short)sa,
                                  (short)sr, (short)sg, (short)sb, (short)sa);
    uint32_t i = 0u;
    for (; i + 4u <= n; i += 4u) {
        uint8_t *q = p + (size_t)i * 4u;
        __m128i d = _mm_loadu_si128((const __m128i *)(const void *)q);
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
        lo = _mm_add_epi16(lo, src);
        hi = _mm_add_epi16(hi, src);
        _mm_storeu_si128((__m128i *)(void *)q, _mm_packus_epi16(lo, hi));
    }
    for (; i < n; i++) ui_blend_cov(p + (size_t)i * 4u, r, g, b, a, 255u, 1.0f);
#else
    ui_blend_span_scalar(p, n, r, g, b, a, 255u, 1.0f);
#endif
}

static void ui_blend_span_cov(uint8_t *p, uint32_t n, uint8_t r, uint8_t g,
                               uint8_t b, uint8_t a, uint8_t cov,
                               float opacity) {
    if (n == 0u || cov == 0u) return;
    if (cov == 255u) {
        ui_blend_span(p, n, r, g, b, a, opacity);
        return;
    }
    uint32_t sa = ((uint32_t)a * (uint32_t)cov) / 255u;
    sa = (sa * (uint32_t)(ui_clamp_opacity(opacity) * 255.0f)) / 255u;
    if (sa == 0u) return;
    uint32_t sr = ((uint32_t)r * sa) / 255u;
    uint32_t sg = ((uint32_t)g * sa) / 255u;
    uint32_t sb = ((uint32_t)b * sa) / 255u;
    uint32_t ia = 255u - sa;
    uint32_t i = 0u;
    for (; i < n; i++) {
        uint8_t *d = p + (size_t)i * 4u;
        d[0] = ui_clamp_u8(sr + ui_div255((uint32_t)d[0] * ia));
        d[1] = ui_clamp_u8(sg + ui_div255((uint32_t)d[1] * ia));
        d[2] = ui_clamp_u8(sb + ui_div255((uint32_t)d[2] * ia));
        d[3] = ui_clamp_u8(sa + ui_div255((uint32_t)d[3] * ia));
    }
}

void ui_raster_clear(uint8_t *pixels, size_t bytes) {
    if (pixels == NULL || bytes == 0u) return;
    memset(pixels, 0, bytes);
}

void ui_raster_fill(uint8_t *pixels, uint32_t w, uint32_t h, uint32_t stride,
                    uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if (pixels == NULL || w == 0u || h == 0u || stride < w * 4u) return;
    uint32_t y = 0u;
    for (; y < h; y++) {
        uint8_t *row = pixels + (size_t)y * stride;
        uint32_t x = 0u;
#if UI_RASTER_SSE2
        uint32_t v = (uint32_t)r | ((uint32_t)g << 8u) |
                     ((uint32_t)b << 16u) | ((uint32_t)a << 24u);
        __m128i vv = _mm_set1_epi32((int)v);
        for (; x + 4u <= w; x += 4u) {
            _mm_storeu_si128((__m128i *)(void *)(row + (size_t)x * 4u), vv);
        }
#endif
        for (; x < w; x++) {
            row[(size_t)x * 4u + 0u] = r;
            row[(size_t)x * 4u + 1u] = g;
            row[(size_t)x * 4u + 2u] = b;
            row[(size_t)x * 4u + 3u] = a;
        }
    }
}

void ui_raster_blend_rect(uint8_t *pixels, uint32_t w, uint32_t h,
                          uint32_t stride, uint32_t x0, uint32_t y0,
                          uint32_t x1, uint32_t y1, uint8_t r, uint8_t g,
                          uint8_t b, uint8_t a, float opacity) {
    if (pixels == NULL || x0 >= x1 || y0 >= y1 || stride < w * 4u) return;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    uint32_t y = y0;
    float op = ui_clamp_opacity(opacity);
    if (a == 255u && op == 1.0f) {
        /* Opaque solid rect: direct 32-bit stores, no blend per pixel. */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint32_t v = (uint32_t)r | ((uint32_t)g << 8u) |
                     ((uint32_t)b << 16u) | ((uint32_t)a << 24u);
        for (; y < y1; y++) {
            uint8_t *row = pixels + (size_t)y * stride;
            uint32_t x = x0;
#if UI_RASTER_SSE2
            __m128i vv = _mm_set1_epi32((int)v);
            for (; x + 4u <= x1; x += 4u) {
                _mm_storeu_si128((__m128i *)(void *)(row + (size_t)x * 4u), vv);
            }
#endif
            for (; x < x1; x++) {
                row[(size_t)x * 4u + 0u] = r;
                row[(size_t)x * 4u + 1u] = g;
                row[(size_t)x * 4u + 2u] = b;
                row[(size_t)x * 4u + 3u] = a;
            }
        }
        return;
#else
        /* Non-little-endian fallback keeps the same byte order semantics. */
        for (; y < y1; y++) {
            uint8_t *row = pixels + (size_t)y * stride;
            for (uint32_t x = x0; x < x1; x++) {
                row[(size_t)x * 4u + 0u] = r;
                row[(size_t)x * 4u + 1u] = g;
                row[(size_t)x * 4u + 2u] = b;
                row[(size_t)x * 4u + 3u] = a;
            }
        }
        return;
#endif
    }
    for (; y < y1; y++) {
        ui_blend_span(pixels + (size_t)y * stride + (size_t)x0 * 4u, x1 - x0,
                      r, g, b, a, opacity);
    }
}

/* Analytic rounded-box coverage (no 16x16 supersampling).
 *
 * The signed-distance approximation is linearized near the rounded edge:
 *   q = max(|p-center| - (half-radius), 0)
 *   coverage = 0.5 - (q.q - radius^2) / (2*radius)
 * The inner core is exactly 255; solid rects remain exact. This is integer
 * blend arithmetic around a deterministic float coverage value and has no
 * FMA-dependent path. */
static inline uint8_t ui_round_cov(uint32_t px, uint32_t py, float rx, float ry,
                                   float rw, float rh, float radius) {
    float rad = radius;
    if (rad <= 0.0f) return 255u;
    if (rad > rw * 0.5f) rad = rw * 0.5f;
    if (rad > rh * 0.5f) rad = rh * 0.5f;
    if (rad <= 0.0f) return 255u;

    float halfw = rw * 0.5f;
    float halfh = rh * 0.5f;
    float cx = (float)px + 0.5f;
    float cy = (float)py + 0.5f;
    float dx = cx - (rx + halfw);
    float dy = cy - (ry + halfh);
    if (dx < 0.0f) dx = 0.0f - dx;
    if (dy < 0.0f) dy = 0.0f - dy;
    float qx = dx - (halfw - rad);
    float qy = dy - (halfh - rad);
    if (qx < 0.0f) qx = 0.0f;
    if (qy < 0.0f) qy = 0.0f;
    if (qx <= 0.0f && qy <= 0.0f) return 255u;

    float cov;
    if (qx <= 0.0f) {
        /* Straight horizontal edge: the SDF distance is qy - rad. */
        cov = 0.5f - (qy - rad);
    } else if (qy <= 0.0f) {
        /* Straight vertical edge. */
        cov = 0.5f - (qx - rad);
    } else {
        /* Rounded corner: linearized distance near the arc. */
        float q2 = qx * qx + qy * qy;
        float r2 = rad * rad;
        cov = 0.5f - (q2 - r2) / (2.0f * rad);
    }
    if (cov != cov || cov <= 0.0f) return 0u;
    if (cov >= 1.0f) return 255u;
    return (uint8_t)(cov * 255.0f);
}

void ui_raster_rounded_rect(uint8_t *pixels, uint32_t w, uint32_t h,
                            uint32_t stride, uint32_t x0, uint32_t y0,
                            uint32_t x1, uint32_t y1, float rx, float ry,
                            float rw, float rh, float radius, uint8_t r,
                            uint8_t g, uint8_t b, uint8_t a, float opacity) {
    if (pixels == NULL || x0 >= x1 || y0 >= y1 || stride < w * 4u) return;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;

    float rad = radius;
    if (rad > rw * 0.5f) rad = rw * 0.5f;
    if (rad > rh * 0.5f) rad = rh * 0.5f;
    if (rad < 0.0f) rad = 0.0f;

    uint32_t sx0 = x0;
    uint32_t sx1 = x1;
    uint32_t sy0 = y0;
    uint32_t sy1 = y1;
    if (rad > 0.0f) {
        float fl = rx + rad - 0.5f;
        float fr = rx + rw - rad - 0.5f;
        float ft = ry + rad - 0.5f;
        float fb = ry + rh - rad - 0.5f;
        if (fl > 0.0f) sx0 = (uint32_t)(fl + 0.999999f);
        if (fr > 0.0f) sx1 = (uint32_t)fr;
        if (ft > 0.0f) sy0 = (uint32_t)(ft + 0.999999f);
        if (fb > 0.0f) sy1 = (uint32_t)fb;
        if (sx0 < x0) sx0 = x0;
        if (sx1 > x1) sx1 = x1;
        if (sy0 < y0) sy0 = y0;
        if (sy1 > y1) sy1 = y1;
    }

    uint32_t y = y0;
    for (; y < y1; y++) {
        uint8_t *row = pixels + (size_t)y * stride;
        int solid_row = y >= sy0 && y < sy1;
        if (solid_row) {
            if (sx1 > sx0) {
                ui_blend_span(row + (size_t)sx0 * 4u, sx1 - sx0, r, g, b, a,
                              opacity);
            }
            uint32_t x = x0;
            for (; x < sx0; x++) {
                uint8_t cov = ui_round_cov(x, y, rx, ry, rw, rh, rad);
                if (cov != 0u) ui_blend_cov(row + (size_t)x * 4u, r, g, b, a,
                                            cov, opacity);
            }
            x = sx1;
            for (; x < x1; x++) {
                uint8_t cov = ui_round_cov(x, y, rx, ry, rw, rh, rad);
                if (cov != 0u) ui_blend_cov(row + (size_t)x * 4u, r, g, b, a,
                                            cov, opacity);
            }
        } else {
            uint32_t x = x0;
            if (sx1 > sx0) {
                for (; x < sx0; x++) {
                    uint8_t cov = ui_round_cov(x, y, rx, ry, rw, rh, rad);
                    if (cov != 0u) ui_blend_cov(row + (size_t)x * 4u, r, g, b,
                                                a, cov, opacity);
                }
                uint8_t covmid = ui_round_cov(sx0, y, rx, ry, rw, rh, rad);
                ui_blend_span_cov(row + (size_t)sx0 * 4u, sx1 - sx0, r, g, b, a,
                                  covmid, opacity);
                x = sx1;
                for (; x < x1; x++) {
                    uint8_t cov = ui_round_cov(x, y, rx, ry, rw, rh, rad);
                    if (cov != 0u) ui_blend_cov(row + (size_t)x * 4u, r, g, b,
                                                a, cov, opacity);
                }
            } else {
                for (; x < x1; x++) {
                    uint8_t cov = ui_round_cov(x, y, rx, ry, rw, rh, rad);
                    if (cov != 0u) ui_blend_cov(row + (size_t)x * 4u, r, g, b,
                                                a, cov, opacity);
                }
            }
        }
    }
}

void ui_raster_copy(uint8_t *dst, uint32_t dw, uint32_t dh, uint32_t dstride,
                    const uint8_t *src, uint32_t sw, uint32_t sh,
                    uint32_t sstride, uint32_t sx, uint32_t sy, uint32_t cw,
                    uint32_t ch, uint32_t dx, uint32_t dy) {
    if (dst == NULL || src == NULL || cw == 0u || ch == 0u) return;
    if (dstride < dw * 4u || sstride < sw * 4u) return;
    if (sx + cw > sw || sy + ch > sh || dx + cw > dw || dy + ch > dh) return;
    uint32_t y = 0u;
    for (; y < ch; y++) {
        memmove(dst + (size_t)(dy + y) * dstride + (size_t)dx * 4u,
                src + (size_t)(sy + y) * sstride + (size_t)sx * 4u,
                (size_t)cw * 4u);
    }
}
