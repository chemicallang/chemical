# Chemical UI — text and font architecture

**Status:** normative, v1 · **Date:** 2026-11-27
**Companions:** [`CONTRACTS.md`](CONTRACTS.md) §10, [`API.md`](API.md) §6,
[`BENCHMARKS.md`](BENCHMARKS.md) §2.3/§2.7, [`EXECUTION.md`](EXECUTION.md)
P2.1–P2.6.

Text is the hardest part of the toolkit. These decisions exist so no implementing
AI has to guess at shaping, fallback, IME, or determinism.

---

## 1. Font file support matrix (v1 minimum)

The in-tree `font` library is a metrics/`glyf` outline parser, not a text
engine. P2.1 must extend it to this matrix, or the platform shaper backend must
return a renderable bitmap. No "system font magically works" assumption.

| Feature | v1 requirement | Owner |
|---------|----------------|-------|
| TTF `glyf`/`loca`, `head`, `hhea`, `hmtx`, `maxp`, `cmap` 4 | **required** | P2.1 |
| `cmap` 12 (non-BMP: emoji, CJK ext) | **required** | P2.1 |
| `OS/2` typo/win metrics, `sxHeight`, `sCapHeight` | **required** | P2.1 |
| `kern` table + GPOS single/pair adjustment | required for Latin | P2.3 |
| Composite glyphs | **required** (accented Latin, CJK, Arabic) | P2.2 |
| TTC collection index | **required** (macOS `.ttc`) | P2.1 |
| CFF/CFF2 OTF | required for system fonts; can use platform raster fallback | P2.5 |
| `name` table (family/style) | **required** for `--ui-font-family` | P2.1 |
| `GSUB` ligatures/`tnum`/`zero` | v1 subset: `liga`, `kern`, `tnum`, `zero`; raw feature tags pass through | P2.3 |
| Variable font axes (`fvar`/`gvar`) | v1.1; v1 uses static faces | P2.11 |
| Color emoji (COLR/CBDT/sbix) | v1.1; v1 falls back to a color-bitmap font or tofu + warning | P2.5 |
| TTF hinting | **off** (deterministic) | P2.2 |

`GlyphOutline` must add `contour_ends : vector<u16>` (and composite component
data or a resolved contour list), otherwise a fill rasterizer cannot close
contours or apply winding. `units_per_em == 0` is rejected. Composite glyphs are
resolved recursively with a depth cap of 8.

### 1.1 Font matching

- Resolution order: app-pinned file → named family (`name` table) → per-script
  fallback list → OS last-resort → tofu.
- Fallback is **per script run**, not one flat list: `FontStack { ui, mono,
  fallback[script] }`. Latin UI, CJK, emoji, and symbol fonts are separate.
- Weight/style matching table (CSS Fonts 4 nearest-neighbour): exact face → same
  weight different style → nearest weight → synthetic bold/oblique with metric
  compensation. Synthetic bold is allowed only if no 600/700 face exists, and is
  reported in `fonts.list`.
- The chosen optional preset is Inter 400 (42 KB Latin subset). It is one
  weight; headings use platform/system semibold or the app pins a multi-weight
  bundle. Do not silently synthesize bold without reporting it.

---

## 2. Metrics and layout

### 2.1 TextMetrics

```chemical
@direct_init
public struct TextLayoutMetrics {
    var width : float
    var height : float
    var first_baseline : float
    var last_baseline : float
    var line_gap : float
    var lines : u32
    var truncated : bool
    var cap_height : float
    var x_height : float
}
```

- Line box = max ascent + max descent + max leading across every fallback font
  in the run. Parse `hhea.lineGap`, `OS/2.sTypoAscender/Descender/LineGap`,
  `usWinAscent/Descent`; do not use `ascent - descent` alone.
- Advances stay in 1/64 px fixed-point (or float) until final quantization; do
  not truncate per glyph before summing.
- `measure_text` is **not** a free function with a global font registry. Use
  `TextContext`:
  ```chemical
  public struct TextContext {
      public func measure(&self, text : string_view, style : TextStyle, constraints : TextConstraints) : TextLayoutMetrics
      public func measure_rich(&self, spans : &[TextSpan], style : TextStyle, constraints : TextConstraints) : TextLayoutMetrics
  }
  ```
  `ViewBuilder.text_context()` and `UiCtx.text_context()` expose it; it is
  `!Send`. A pre-run measurement is not part of the public API.
- `TextConstraints { max_width, max_lines, wrap, overflow, tab_size, direction }`.

### 2.2 Cache key (exact)

```chemical
@direct_init
public struct TextLayoutKey {
    var text_hash : u64
    var style_hash : u64
    var font_epoch : u32        // bumps on font stack/theme/scale change
    var scale_q : u32           // 1/64 px
    var max_width_q : u32       // 1/64 px
    var max_lines : u16
    var tab_size : u8
    var direction : Direction
    var features : u64
}
```

- Cache shaping (text+font+features) separately from line breaking
  (width+wrap+max_lines). A column resize changes only the break cache.
- Bounded LRU with a byte budget; per-paragraph runs are evicted as a unit.
- Theme/font/scale change bumps `font_epoch` and invalidates all layout caches.

---

## 3. Shaping and Unicode

| Tier | Scripts | Engine |
|------|---------|--------|
| Own shaper | Latin, Greek, Cyrillic | `font` cmap/hmtx/kern/GPOS pair, `liga` |
| Platform shaper | Arabic, Hebrew, Indic, Thai, CJK, emoji | DirectWrite / CoreText / HarfBuzz (Linux) |
| Browser shaper | WASM | Canvas2D `fillText` + `measureText` by default; pinned-font path for deterministic tests |

- Shaped output:
  ```chemical
  @direct_init
  public struct ShapedGlyph {
      var face : FontHandle
      var glyph_id : u32
      var cluster : u32
      var advance_q : i32
      var offset_x_q : i32
      var offset_y_q : i32
      var flags : u32
  }
  ```
- Platform shapers must expose the font file + face index, or return a
  platform-rendered bitmap (CFF/TTC/variable). Our atlas never guesses a glyph
  ID for a different font file.
- Incremental shaping: only the dirty paragraph is re-shaped; a keystroke in a
  2k-character paragraph re-shapes the containing paragraph, not the document.
- **Unicode data is a v1 dependency.** Choose tables over ICU for determinism:
  - UAX #29 grapheme/word boundaries (generated tables, bounded size).
  - UAX #14 line breaking (generated tables; CJK + Latin).
  - UAX #9 bidi (generated tables).
  - NFC/NFD normalization + casefold for search; normalize text at input
    boundaries (store NFC by default, report raw in debug).
  - Data size is reported in the binary budget; a compact generated table set is
    expected (single-digit KB–low hundreds of KB, measured).
- Tabs: `tab_size` spaces; default 4. Letter spacing is applied between
  grapheme clusters (not after the last) and affects wrapping/ellipsis.
- Feature set: `TextFeatures { ligatures, tabular_numbers, slashed_zero,
  stylistic_set : u8, raw_tags : &[u32] }`; not a `u32` bitmask.

### 3.1 Bidi and direction

- Paragraph-level base direction from `Direction`/locale; runs resolved with
  UAX #9. RTL mirrors `Start/End`, padding, icons with `mirror` metadata, and
  caret movement. Mixed Arabic/Latin is tested.
- Line breaking for CJK/Thai uses UAX #14; no Latin mid-word breaks unless
  `word_break: BreakWord`.

---

## 4. IME and editing

### 4.1 State

```chemical
@direct_init
public struct TextEditState {
    var text : string_view        // borrowed view of runtime-owned text
    var anchor : u32              // byte offset
    var caret : u32
    var composition : Option<ImeComposition>
    var secure : bool
}
@direct_init
public struct ImeComposition {
    var preedit : string_view
    var caret : u32               // byte offset within preedit
    var anchor : u32
    var replace : Range            // document range replaced
    var candidate : Option<Rect>   // logical px, window coords
}
```

`UiCtx` exposes:

```chemical
public func text_state(&self, key : Key) : Option<TextEditState>
public func set_text(&mut self, key : Key, text : string_view)
public func set_selection(&mut self, key : Key, anchor : u32, caret : u32)
public func select_all(&mut self, key : Key)
public func ime_rect(&mut self, key : Key, rect : Rect)
```

Controlled/uncontrolled contract:

- `TextInputProps.value` (controlled) or `default_value` (uncontrolled) — exactly
  one; controlled means the app supplies the value each `view` and the runtime
  never diverges. Uncontrolled means the runtime owns it; `ctx.set_text` is the
  reset API.
- `Change` fires with `Value { text, range }` after a commit (not each preedit).
  `Submit` carries the final text. Copying text into app state on every keystroke
  is the app's choice; `ctx.take_text(key)` moves the string without a copy.
- Password fields set `secure = true`; secure values are redacted from
  `tree.snapshot`, `input.text` echoes, logs, a11y `value`, capture, and
  `UiCtx.text` (returns `"••••"`). A debug-only `--ui-reveal-secrets` is required
  to inspect them and must be off by default.
- Undo groups: one per IME commit, paste, word/line delete, and typing burst
  (500 ms). `SelectionChanged`/`TextChanged { range, inserted }` are events.

### 4.2 Host IME

- GTK: `GtkIMContext` preedit/commit, `gtk_im_context_set_cursor_location`.
- Win32: `WM_IME_STARTCOMPOSITION/COMPOSITION/ENDCOMPOSITION`,
  `ImmSetCandidateWindow`, UTF-16 surrogate pairs delivered as whole UTF-8.
- Cocoa: `NSTextInputClient` (first responder), `setMarkedText`,
  `firstRectForCharacterRange`.
- WASM: hidden `<textarea>`/`contenteditable`, `compositionstart/update/end`,
  `selectionStart/End`, `getBoundingClientRect` for candidate rect.
- Requirement: no lost/duplicated characters for a 1k-event mixed UTF-8/IME
  fuzz; preedit never commits twice; cancel restores the pre-edit range.

---

## 5. Rendering and determinism

- Alpha-only R8 glyph atlas; no color in the atlas.
- Subpixel bins: 4 for <20 px, 1 for ≥20 px; baseline snapped to 1/64 px.
- Hinting off; grayscale AA; sRGB-encoded integer blending (see CONTRACTS §6).
- Atlas is grow-only within a frame; LRU eviction at frame boundaries;
  `atlas_generation` invalidates dependent tile/layer/GPU caches and is reported
  as a full repaint.
- Atlas budget: 16 MB default. 5k glyphs at 4 bins/2× stays within budget; if
  not, reduce bins by size and report the quality change.
- CPU and GPU use the same glyph bitmaps and positions; GPU uploads only the
  atlas delta. WASM Canvas2D text is a different backend and tolerance-only.

### 5.1 WASM text

WASM has two backends:

- **BrowserCanvas** (default): `measureText`/`fillText`, fonts via
  `document.fonts`/`FontFace`, IME via hidden textarea. Metrics are
  browser-dependent; goldens are tolerance-based; no bit-exact claim.
- **PinnedFont** (deterministic tests/demos): a downloaded/embedded font,
  browser shaping, canvas draw; layout uses our metrics where possible. Still
  not bit-identical to native, but much closer.
- First layout waits for `document.fonts.ready` (or a font-load timeout) and
  reports a `FontWarning`; never lay out with fallback metrics and silently
  reflow.

---

## 6. Accessibility text

- `Semantics` must carry `alt`, `lang`, `placeholder`, `error`, and `secure`.
- `TextRole.H1–H4` automatically maps to `Role.Heading` with `level`.
- Reading order follows logical text order; RTL/bidi order is reported.
- Secure text is never exposed to the a11y tree; value is `"••••"` or omitted.
- Rich text spans must expose links/emphasis semantically, not only visually.
- 200% text reflows (not just scales); no clipping in the component gallery.

---

## 7. Licensing and assets

- System fonts (Segoe UI, SF Pro, etc.) are used via OS APIs only; never copied.
- OFL/Apache/BSD fonts may be downloaded/embedded with their license and
  attribution. `FontCacheEntry { license, source_url, version }`.
- The 42 KB Inter Latin subset is OFL; embedding it in an app/examples repo
  requires `LICENSES/` + notice. `use_inter_latin()` downloads to cache by
  default; the compiler ships no font.
- Screenshots/goldens containing OFL glyphs may be published with attribution.
- `P8.1` audits every embedded font/icon/asset license.

---

## 8. Test oracle and goldens

- Own shaper: expected advances from a pinned reference build of the same
  shaper, committed as JSON; not generated at test time. HarfBuzz (platform
  oracle) is only for cross-checking.
- Fixtures: a small OFL TTF with SHA-256, a composite-glyph sample (`é`), a CJK
  sample, a `.ttc`, and a corrupt font. Commit under `lang/tests/ui/assets/`.
- Goldens: 1×/2×, 11/13/16/24/32 px, Latin/Greek/Cyrillic/Arabic/Devanagari/
  CJK/emoji, LTR/RTL, ellipsis, tabs, max-lines, secure. Pinned font is
  bit-exact; system/browser goldens are tolerance-only.
- Tests must cover: font missing, glyph missing, CFF/TTC, `units_per_em=0`,
  composite depth overflow, 100k-row digit scroll cache hit, 2k-char typing,
  IME preedit/commit/cancel, password redaction, NFD/NFC equality, ZWJ deletion.
