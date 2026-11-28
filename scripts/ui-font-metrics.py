#!/usr/bin/env python3
"""Deterministic font metrics oracle for the P1.5b pinned test font.

Parses the sfnt tables directly (no fontTools). `--check` regenerates the
metrics in memory and fails if `lang/tests/ui/assets/metrics.json` is stale.

The resolved metrics follow the P1.5b reader contract:
  ascent  = max(hhea.ascender, OS/2.sTypoAscender, OS/2.usWinAscent)
  descent = max(-hhea.descender, -OS/2.sTypoDescender, OS/2.usWinDescent)
  line_gap= max(hhea.lineGap, OS/2.sTypoLineGap)
  cap/x   = OS/2.sCapHeight / OS/2.sxHeight (0 when absent)
"""
import hashlib
import json
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSET_DIR = os.path.join(ROOT, "lang/tests/ui/assets")
FONT_PATH = os.path.join(ASSET_DIR, "fonts/LiberationSans-Regular.ttf")
OUT_PATH = os.path.join(ASSET_DIR, "metrics.json")
CODEPOINTS = [0x20, 0x30, 0x31, 0x41, 0x42, 0x48, 0x64, 0x65, 0x6C, 0x6F,
              0x72, 0x77, 0xE9, 0x4E2D, 0x1F600]


def fail(msg):
    print("ui-font-metrics: " + msg, file=sys.stderr)
    sys.exit(1)


def u16(d, o):
    if o + 2 > len(d):
        fail("truncated u16")
    return struct.unpack_from(">H", d, o)[0]


def i16(d, o):
    return struct.unpack_from(">h", d, o)[0]


def u32(d, o):
    if o + 4 > len(d):
        fail("truncated u32")
    return struct.unpack_from(">I", d, o)[0]


def tables(d):
    if len(d) < 12 or u32(d, 0) not in (0x00010000, 0x4F54544F):
        fail("not a TrueType/OpenType font")
    n = u16(d, 4)
    out = {}
    for i in range(n):
        p = 12 + i * 16
        if p + 16 > len(d):
            fail("truncated table directory")
        tag = d[p:p + 4].decode("latin1")
        out[tag] = (u32(d, p + 8), u32(d, p + 12))
    return out


def cmap4_lookup(d, cmap_off, cp):
    if cmap_off + 4 > len(d):
        return 0
    n = u16(d, cmap_off + 2)
    sub = None
    for i in range(n):
        p = cmap_off + 4 + i * 8
        if p + 8 > len(d):
            break
        platform = u16(d, p)
        encoding = u16(d, p + 2)
        off = u32(d, p + 4)
        if platform == 3 and encoding == 1:
            sub = off
            break
        if platform == 0 and sub is None:
            sub = off
    if sub is None:
        return 0
    base = cmap_off + sub
    if base + 8 > len(d) or u16(d, base) != 4:
        return 0
    seg = u16(d, base + 6) // 2
    end_base = base + 14
    start_base = base + 16 + 2 * seg
    delta_base = base + 16 + 4 * seg
    range_base = base + 16 + 6 * seg
    for i in range(seg):
        eo = end_base + 2 * i
        so = start_base + 2 * i
        do = delta_base + 2 * i
        ro = range_base + 2 * i
        if max(eo, so, do, ro) + 2 > len(d):
            return 0
        end = u16(d, eo)
        start = u16(d, so)
        if cp < start or cp > end:
            continue
        delta = i16(d, do)
        off = u16(d, ro)
        if off == 0:
            return (cp + delta) & 0xFFFF
        addr = ro + off + 2 * (cp - start)
        if addr + 2 > len(d):
            return 0
        gid = u16(d, addr)
        return ((gid + delta) & 0xFFFF) if gid else 0
    return 0


def advance_for(d, tabs, gid):
    if "hmtx" not in tabs or "hhea" not in tabs:
        return 0
    hmtx = tabs["hmtx"][0]
    hhea = tabs["hhea"][0]
    if hhea + 36 > len(d):
        return 0
    n_long = u16(d, hhea + 34)
    if n_long == 0:
        return 0
    if gid < n_long:
        p = hmtx + 4 * gid
    else:
        p = hmtx + 4 * (n_long - 1)
    if p + 2 > len(d):
        return 0
    return u16(d, p)


def main():
    check = "--check" in sys.argv[1:]
    unknown = [a for a in sys.argv[1:] if a != "--check"]
    if unknown:
        fail("unknown argument " + " ".join(unknown))
    d = open(FONT_PATH, "rb").read()
    tabs = tables(d)
    for required in ("head", "hhea", "maxp", "hmtx", "cmap"):
        if required not in tabs:
            fail("missing required table " + required)
    upem = u16(d, tabs["head"][0] + 18)
    if upem == 0:
        fail("units_per_em is zero")
    hhea = tabs["hhea"][0]
    asc = i16(d, hhea + 4)
    desc = -i16(d, hhea + 6)
    gap = i16(d, hhea + 8)
    cap = 0
    xheight = 0
    if "OS/2" in tabs:
        o = tabs["OS/2"][0]
        if o + 78 <= len(d):
            typo_asc = i16(d, o + 68)
            typo_desc = i16(d, o + 70)
            typo_gap = i16(d, o + 72)
            win_asc = u16(d, o + 74)
            win_desc = u16(d, o + 76)
            asc = max(asc, typo_asc, win_asc)
            desc = max(desc, -typo_desc, win_desc)
            gap = max(gap, typo_gap)
            if o + 90 <= len(d):
                xheight = i16(d, o + 86)
                cap = i16(d, o + 88)
    cmap_off = tabs["cmap"][0]
    advances = []
    for cp in CODEPOINTS:
        gid = cmap4_lookup(d, cmap_off, cp)
        adv = advance_for(d, tabs, gid)
        advances.append({"codepoint": cp, "glyph": gid,
                         "advance": adv, "missing": gid == 0})
    data = {
        "schema": 1,
        "font_file": "fonts/LiberationSans-Regular.ttf",
        "font_sha256": hashlib.sha256(d).hexdigest(),
        "family": "Liberation Sans",
        "style": "Regular",
        "version": "2.1.5",
        "license": "OFL-1.1",
        "units_per_em": upem,
        "metrics": {
            "ascent": asc,
            "descent": desc,
            "line_gap": gap,
            "line_height": asc + desc + gap,
            "cap_height": cap,
            "x_height": xheight,
        },
        "fallback_advance_em": 0.5,
        "advances": advances,
    }
    text = json.dumps(data, indent=2, sort_keys=True) + "\n"
    if check:
        if not os.path.isfile(OUT_PATH):
            fail("missing " + OUT_PATH)
        if open(OUT_PATH, encoding="utf-8").read() != text:
            fail("metrics.json is stale; run scripts/ui-font-metrics.py")
        print("ui-font-metrics: metrics.json matches the font tables")
        return 0
    with open(OUT_PATH, "w", encoding="utf-8") as f:
        f.write(text)
    print("ui-font-metrics: wrote " + OUT_PATH)
    return 0


if __name__ == "__main__":
    sys.exit(main())
