#!/usr/bin/env python3
"""Generate the P0.4 static design references from tokens.json.

The committed HTML files are generated; templates hold layout only and the
inline CSS variable block is derived from lang/docs/ui/tokens.json. `--check`
verifies there is no drift. No renderer dependency is used.
"""
import html
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOKENS = os.path.join(ROOT, "lang/docs/ui/tokens.json")
DESIGN = os.path.join(ROOT, "lang/docs/ui/design")
TEMPLATES = os.path.join(DESIGN, "templates")
MARKER = "/*__DESIGN_INLINE__*/"
SHEET_MARKER = "<!--__TOKEN_SHEET__-->"
FILES = {
    "settings.html": "reference-settings.html",
    "dashboard.html": "reference-dashboard.html",
    "chat.html": "reference-chat.html",
    "tokens.html": "tokens.html",
}

BASE_CSS = r"""
* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; }
body {
  width: 1280px; min-height: 800px;
  background: var(--color-surface);
  color: var(--color-on-surface);
  font-family: var(--font-ui);
  font-size: var(--text-body-size);
  line-height: var(--text-body-line);
  overflow: hidden;
}
a { color: var(--color-primary); text-decoration: underline; }
button, input, select, textarea { font: inherit; }
button { cursor: pointer; }
.app { display: flex; width: 1280px; min-height: 800px; }
.sidebar {
  width: var(--sidebar-lg); flex: 0 0 var(--sidebar-lg);
  background: var(--color-surface-raised);
  border-right: 1px solid var(--color-border);
  padding: var(--space-lg); display: flex; flex-direction: column;
  gap: var(--space-lg);
}
.main { flex: 1; min-width: 0; padding: var(--page-padding); }
.brand { display: flex; align-items: center; gap: var(--space-sm); font-weight: 700; }
.brand-mark {
  width: 28px; height: 28px; border-radius: var(--radius-sm);
  background: var(--color-primary); color: var(--color-on-primary);
  display: grid; place-items: center; font-size: 14px; font-weight: 700;
}
.brand-sub { color: var(--color-on-surface-faint); font-size: var(--text-caption-size); line-height: var(--text-caption-line); }
.nav { display: flex; flex-direction: column; gap: var(--space-xxs); }
.nav-label { color: var(--color-on-surface-faint); font-size: var(--text-caption-size); line-height: var(--text-caption-line); letter-spacing: var(--text-caption-letter); text-transform: uppercase; font-weight: 600; margin-top: var(--space-sm); }
.nav-item {
  display: flex; align-items: center; gap: var(--space-sm);
  min-height: var(--control-sm); padding: 0 var(--space-sm);
  border-radius: var(--radius-sm); color: var(--color-on-surface-muted);
  text-decoration: none; border: 1px solid transparent;
}
.nav-item.active { background: var(--color-selection); color: var(--color-on-surface); font-weight: 600; }
.nav-item .dot { width: 8px; height: 8px; border-radius: var(--radius-full); background: var(--color-primary); }
.nav-item .dot.muted { background: var(--color-on-surface-faint); }
.sidebar-footer {
  margin-top: auto; border-top: 1px solid var(--color-border); padding-top: var(--space-md);
  display: flex; align-items: center; gap: var(--space-sm);
}
.avatar {
  width: 32px; height: 32px; border-radius: var(--radius-full);
  display: grid; place-items: center; font-weight: 700; font-size: 12px;
  background: var(--color-selection); color: var(--color-on-surface);
}
.avatar.primary { background: var(--color-primary); color: var(--color-on-primary); }
.avatar.danger { background: var(--color-danger); color: var(--color-on-danger); }
.topbar { display: flex; align-items: center; justify-content: space-between; gap: var(--space-lg); margin-bottom: var(--space-lg); }
.title { font-size: var(--text-h1-size); line-height: var(--text-h1-line); font-weight: var(--text-h1-weight); letter-spacing: var(--text-h1-letter); margin: 0; }
.subtitle { color: var(--color-on-surface-muted); margin: var(--space-xxs) 0 0; }
.row { display: flex; align-items: center; gap: var(--space-md); }
.row.wrap { flex-wrap: wrap; }
.spacer { flex: 1; }
.search {
  display: flex; align-items: center; gap: var(--space-sm);
  min-width: 320px; height: var(--control-md); padding: 0 var(--space-md);
  background: var(--color-surface-raised); border: 1px solid var(--color-border);
  border-radius: var(--radius-sm); color: var(--color-on-surface-faint);
}
.button {
  display: inline-flex; align-items: center; justify-content: center; gap: var(--space-sm);
  min-height: var(--control-md); padding: 0 var(--space-lg);
  border-radius: var(--radius-md); border: 1px solid var(--color-border-strong);
  background: var(--color-surface-raised); color: var(--color-on-surface); font-weight: 600;
}
.button.primary { background: var(--color-primary); border-color: var(--color-primary); color: var(--color-on-primary); }
.button.secondary { background: var(--color-secondary); border-color: var(--color-border); color: var(--color-on-secondary); }
.button.danger { background: var(--color-danger); border-color: var(--color-danger); color: var(--color-on-danger); }
.button:disabled { opacity: var(--disabled-opacity); }
.focus-ring { outline: var(--focus-width) solid var(--color-focus-ring); outline-offset: var(--focus-offset); }
.card {
  background: var(--color-surface-raised); border: 1px solid var(--color-border);
  border-radius: var(--radius-md); box-shadow: var(--shadow-low);
}
.card-pad { padding: var(--space-lg); }
.h2 { font-size: var(--text-h2-size); line-height: var(--text-h2-line); font-weight: var(--text-h2-weight); letter-spacing: var(--text-h2-letter); margin: 0; }
.h3 { font-size: var(--text-h3-size); line-height: var(--text-h3-line); font-weight: var(--text-h3-weight); margin: 0; }
.h4 { font-size: var(--text-h4-size); line-height: var(--text-h4-line); font-weight: var(--text-h4-weight); margin: 0; }
.muted { color: var(--color-on-surface-muted); }
.faint { color: var(--color-on-surface-faint); }
.caption { font-size: var(--text-caption-size); line-height: var(--text-caption-line); letter-spacing: var(--text-caption-letter); color: var(--color-on-surface-faint); }
.label { font-size: var(--text-label-size); line-height: var(--text-label-line); font-weight: var(--text-label-weight); }
.mono { font-family: var(--font-mono); font-size: var(--text-code-size); line-height: var(--text-code-line); }
.badge {
  display: inline-flex; align-items: center; gap: var(--space-xxs);
  min-height: 22px; padding: 0 var(--space-xs); border-radius: var(--radius-xs);
  background: var(--color-surface-sunken); color: var(--color-on-surface-muted);
  font-size: var(--text-caption-size); line-height: var(--text-caption-line); font-size: var(--text-caption-size);
}
.badge.success { background: var(--color-success); color: var(--color-on-success); }
.badge.warning { background: var(--color-warning); color: var(--color-on-warning); }
.badge.danger { background: var(--color-danger); color: var(--color-on-danger); }
.badge.primary { background: var(--color-selection); color: var(--color-on-surface); }
.divider { height: 1px; background: var(--color-border); border: 0; margin: var(--space-md) 0; }
.field { display: flex; flex-direction: column; gap: var(--space-xs); }
.field label { font-size: var(--text-label-size); line-height: var(--text-label-line); font-weight: var(--text-label-weight); }
.input, .select {
  height: var(--control-md); border: 1px solid var(--color-border-strong);
  border-radius: var(--radius-sm); background: var(--color-surface);
  color: var(--color-on-surface); padding: 0 var(--space-md);
  display: flex; align-items: center;
}
.input.invalid { border: 2px solid var(--color-danger); }
.helper { font-size: var(--text-caption-size); line-height: var(--text-caption-line); color: var(--color-on-surface-faint); }
.error-text { font-size: var(--text-caption-size); line-height: var(--text-caption-line); color: var(--color-danger); }
.switch { width: 34px; height: 20px; border-radius: var(--radius-full); background: var(--color-primary); position: relative; }
.switch::after { content: ""; position: absolute; right: 2px; top: 2px; width: 16px; height: 16px; border-radius: var(--radius-full); background: var(--color-on-primary); }
.switch.off { background: var(--color-border-strong); }
.switch.off::after { right: auto; left: 2px; background: var(--color-surface-raised); }
.table { width: 100%; border-collapse: collapse; }
.table th { text-align: left; height: var(--table-header); color: var(--color-on-surface-faint); font-size: var(--text-caption-size); font-weight: 600; border-bottom: 1px solid var(--color-border); padding: 0 var(--space-sm); }
.table td { height: var(--table-row); border-bottom: 1px solid var(--color-border); padding: 0 var(--space-sm); }
.table tr:last-child td { border-bottom: 0; }
.progress { height: 8px; border-radius: var(--radius-full); background: var(--color-surface-sunken); overflow: hidden; }
.progress > span { display: block; height: 100%; border-radius: var(--radius-full); background: var(--color-primary); }
.metric { font-size: var(--text-h2-size); line-height: var(--text-h2-line); font-weight: 700; letter-spacing: var(--text-h2-letter); }
.trend-up { color: var(--color-success); font-weight: 600; }
.trend-down { color: var(--color-danger); font-weight: 600; }
.sr-only { position: absolute; width: 1px; height: 1px; overflow: hidden; clip: rect(0,0,0,0); }
"""


def css_name(name):
    # OnSurface -> on-surface, XXS -> xxs, Chart1 -> chart1
    name = re.sub(r"(?<=[a-z0-9])([A-Z])", r"-\1", name)
    name = re.sub(r"([A-Z]+)([A-Z][a-z])", r"\1-\2", name)
    return name.lower()


def shadow_css(layers):
    if not layers:
        return "none"
    parts = []
    for l in layers:
        parts.append(f'{l["x"]}px {l["y"]}px {l["blur"]}px {l["spread"]}px {l["color"]}')
    return ", ".join(parts)


def colors_block(tokens, mode):
    return "\n".join(
        f"  --color-{css_name(name)}: {value};"
        for name, value in tokens["colors"]["modes"][mode].items()
    )


def spacing_block(tokens, density):
    return "\n".join(
        f"  --space-{css_name(name)}: {px}px;"
        for name, px in tokens["spacing"][density].items()
    )


def density_block(tokens, density):
    d = tokens["density"][density]
    lines = [
        f"  --control-sm: {d['control_small']}px;",
        f"  --control-md: {d['control_medium']}px;",
        f"  --control-lg: {d['control_large']}px;",
        f"  --table-row: {d['table_row']}px;",
        f"  --table-header: {d['table_header']}px;",
        f"  --sidebar-lg: {d['sidebar_lg']}px;",
        f"  --sidebar-xl: {d['sidebar_xl']}px;",
    ]
    return "\n".join(lines)


def inline_css(tokens):
    light = tokens["colors"]["modes"]["light"]
    dark = tokens["colors"]["modes"]["dark"]
    hc = tokens["colors"]["modes"]["high-contrast"]
    focus = tokens["focus"]
    roles = tokens["typography"]["roles"]
    motion = tokens["motion"]
    layout = tokens["layout"]
    b = tokens["breakpoints"]
    lines = ["/* Generated from lang/docs/ui/tokens.json. Do not edit. */", ":root {"]
    lines.append(colors_block(tokens, "light"))
    lines.append(f"  --focus-width: {focus['light']['width']}px;")
    lines.append(f"  --focus-offset: {focus['light']['offset']}px;")
    lines.append(f"  --border-width: {focus['border_width']['light']}px;")
    lines.append(f"  --selected-border-width: {layout['selected_border_width']}px;")
    lines.append(f"  --disabled-opacity: {tokens['disabled_opacity']};")
    lines.append(f"  --font-ui: {tokens['typography']['font_stack']};")
    lines.append(f"  --font-mono: {tokens['typography']['mono_stack']};")
    lines.append(f"  --content-max: {tokens['typography']['max_content_width']}px;")
    lines.append(f"  --prose-max: {tokens['typography']['prose_ch']}ch;")
    lines.append(f"  --table-max: {tokens['typography']['table_ch']}ch;")
    lines.append(f"  --min-touch: {layout['min_touch']}px;")
    lines.append(f"  --preferred-touch: {layout['preferred_touch']}px;")
    for name, spec in roles.items():
        n = css_name(name)
        lines.append(f"  --text-{n}-size: {spec['size']}px;")
        lines.append(f"  --text-{n}-line: {spec['line']}px;")
        lines.append(f"  --text-{n}-weight: {spec['weight']};")
        lines.append(f"  --text-{n}-letter: {spec['letter_spacing']}em;")
        lines.append(f"  --text-{n}-family: var(--font-{'mono' if spec['family'] == 'Mono' else 'ui'});")
    for name, px in tokens["radii"].items():
        lines.append(f"  --radius-{css_name(name)}: {px}px;")
    lines.append(spacing_block(tokens, "comfortable"))
    lines.append(density_block(tokens, "Comfortable"))
    for name in ("None", "Low", "Medium", "High"):
        lines.append(f"  --shadow-{css_name(name)}: {shadow_css(tokens['elevation']['light'][name]['layers'])};")
    for name in ("Fast", "Base", "Slow", "Spring"):
        lines.append(f"  --motion-{css_name(name)}: {motion[name]['ms']}ms;")
    for name, value in motion["easings"].items():
        lines.append(f"  --ease-{name.replace('_','-')}: {value};")
    lines.append(f"  --bp-sm-max: {b['Sm']['max']}px;")
    lines.append(f"  --bp-md-min: {b['Md']['min']}px;")
    lines.append(f"  --bp-md-max: {b['Md']['max']}px;")
    lines.append(f"  --bp-lg-min: {b['Lg']['min']}px;")
    lines.append(f"  --bp-lg-max: {b['Lg']['max']}px;")
    lines.append(f"  --bp-xl-min: {b['Xl']['min']}px;")
    lines.append("  --page-padding: 32px;")
    lines.append("}")
    lines.append("")
    lines.append('[data-theme="dark"] {')
    lines.append(colors_block(tokens, "dark"))
    lines.append(f"  --focus-width: {focus['dark']['width']}px;")
    lines.append(f"  --focus-offset: {focus['dark']['offset']}px;")
    lines.append(f"  --border-width: {focus['border_width']['dark']}px;")
    for name in ("None", "Low", "Medium", "High"):
        lines.append(f"  --shadow-{css_name(name)}: {shadow_css(tokens['elevation']['dark'][name]['layers'])};")
    lines.append("}")
    lines.append("")
    lines.append('[data-theme="high-contrast"] {')
    lines.append(colors_block(tokens, "high-contrast"))
    lines.append(f"  --focus-width: {focus['high-contrast']['width']}px;")
    lines.append(f"  --focus-offset: {focus['high-contrast']['offset']}px;")
    lines.append(f"  --border-width: {focus['border_width']['high-contrast']}px;")
    for name in ("None", "Low", "Medium", "High"):
        lines.append(f"  --shadow-{css_name(name)}: {shadow_css(tokens['elevation']['high-contrast'][name]['layers'])};")
    lines.append("}")
    lines.append("")
    lines.append('[data-density="compact"] {')
    lines.append(spacing_block(tokens, "compact"))
    lines.append(density_block(tokens, "Compact"))
    lines.append("}")
    lines.append(BASE_CSS)
    return "\n".join(lines)


def token_sheet(tokens):
    modes = tokens["colors"]["modes"]
    out = []
    out.append('<section class="sheet-section">')
    out.append('<div class="h2">Color tokens</div><p class="muted">Every ColorToken, by mode. Values are the generated CSS variables.</p>')
    out.append('<div class="swatch-modes">')
    for mode_name, mode in (("Light", "light"), ("Dark", "dark"), ("High contrast", "high-contrast")):
        out.append('<div class="swatch-column">')
        out.append(f'<div class="label">{mode_name}</div><div class="swatches">')
        for name, value in modes[mode].items():
            css = f"var(--color-{css_name(name)})"
            out.append(f'<div class="swatch"><span class="chip" style="background:{css}"></span><span class="swatch-name">{html.escape(name)}</span><span class="mono faint">{html.escape(value)}</span></div>')
        out.append("</div></div>")
    out.append("</div></section>")

    out.append('<section class="sheet-section">')
    out.append('<div class="h2">Type ramp</div><p class="muted">System UI stack; mono for code. Sizes and line heights are exact.</p><div class="type-ramp">')
    for name, spec in tokens["typography"]["roles"].items():
        n = css_name(name)
        out.append(f'<div class="type-row"><div class="type-sample" style="font-size:var(--text-{n}-size);line-height:var(--text-{n}-line);font-weight:var(--text-{n}-weight);letter-spacing:var(--text-{n}-letter);font-family:var(--text-{n}-family)">Aa The quick brown fox</div><div class="type-meta"><b>{html.escape(name)}</b><span class="mono faint">{spec["size"]}/{spec["line"]} · {spec["weight"]} · {spec["letter_spacing"]}em</span></div></div>')
    out.append("</div></section>")

    out.append('<section class="sheet-section">')
    out.append('<div class="h2">Spacing and density</div><p class="muted">8-pt grid; compact scale is audit-allowed. Comfortable and compact are side by side.</p>')
    out.append('<div class="two-col"><div><div class="label">Comfortable</div>')
    for name, px in tokens["spacing"]["comfortable"].items():
        out.append(f'<div class="scale-row"><span class="mono faint">{name}</span><span class="scale-bar" style="width:{max(2, px*8)}px;background:var(--color-primary)"></span><span class="mono">{px}px</span></div>')
    out.append('</div><div><div class="label">Compact</div>')
    for name, px in tokens["spacing"]["compact"].items():
        out.append(f'<div class="scale-row"><span class="mono faint">{name}</span><span class="scale-bar" style="width:{max(2, px*8)}px;background:var(--color-chart2)"></span><span class="mono">{px}px</span></div>')
    out.append('</div></div>')
    out.append('<div class="density-table"><div><b>Comfortable controls</b><span class="mono">32 / 36 / 40</span><span class="faint">table 36 / header 40</span></div><div><b>Compact controls</b><span class="mono">28 / 32 / 36</span><span class="faint">table 32 / header 36</span></div><div><b>Breakpoints</b><span class="mono">Sm &lt;640 · Md 640–1023 · Lg 1024–1439 · Xl ≥1440</span></div></div>')
    out.append("</section>")

    out.append('<section class="sheet-section">')
    out.append('<div class="h2">Radii, elevation, focus</div><div class="three-col">')
    out.append('<div><div class="label">Radii</div>')
    for name, px in tokens["radii"].items():
        out.append(f'<div class="scale-row"><span class="mono faint">{name}</span><span style="width:48px;height:22px;background:var(--color-selection);border:1px solid var(--color-border-strong);border-radius:{px}px"></span><span class="mono">{px}px</span></div>')
    out.append('</div><div><div class="label">Elevation</div>')
    for name in ("Low", "Medium", "High"):
        out.append(f'<div class="elev-card" style="box-shadow:var(--shadow-{css_name(name)})">{name}</div>')
    out.append('</div><div><div class="label">Focus ring</div><div class="focus-demo focus-ring">2px ring / 2px offset</div><div data-theme="high-contrast" class="focus-demo focus-ring hc" style="background:var(--color-surface);color:var(--color-on-surface)">3px ring high contrast</div><div class="caption">FocusRing vs Surface ≥ 3:1 in every mode.</div><button class="button primary" disabled style="margin-top:8px">Disabled 40%</button></div>')
    out.append("</div></section>")

    out.append('<section class="sheet-section">')
    out.append('<div class="h2">Motion</div><p class="muted">120–240 ms, reduced motion collapses to the end state.</p>')
    for name in ("Fast", "Base", "Slow", "Spring"):
        out.append(f'<div class="scale-row"><span class="mono faint">{name}</span><span class="motion-track"><span class="motion-fill" style="width:{tokens["motion"][name]["ms"]*1.5}px;background:var(--color-chart4)"></span></span><span class="mono">{tokens["motion"][name]["ms"]}ms</span></div>')
    out.append('<div class="caption">Ease-out cubic-bezier(0,0,0.2,1) · ease-in cubic-bezier(0.4,0,1,1) · ease-in-out cubic-bezier(0.4,0,0.2,1).</div>')
    out.append("</section>")
    return "\n".join(out)


def token_sheet_page(tokens):
    css = inline_css(tokens)
    body = token_sheet(tokens)
    extra = r"""
  body { height: auto; overflow: visible; }
  .sheet { width: 1280px; min-height: 1500px; padding: 40px 48px; background: var(--color-surface); }
  .sheet-section { margin-bottom: 32px; padding: 24px; border: 1px solid var(--color-border); border-radius: var(--radius-lg); background: var(--color-surface-raised); box-shadow: var(--shadow-low); }
  .swatch-modes { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; }
  .swatches { display: grid; grid-template-columns: 1fr; gap: 4px 12px; margin-top: 8px; }
  .swatch { display: flex; align-items: center; gap: 6px; min-height: 24px; }
  .swatch-name { width: 96px; font-size: 12px; }
  .chip { width: 20px; height: 20px; border-radius: var(--radius-xs); border: 1px solid var(--color-border-strong); flex: 0 0 20px; }
  .type-ramp { display: flex; flex-direction: column; gap: 10px; margin-top: 12px; }
  .type-row { display: grid; grid-template-columns: 1fr 240px; align-items: baseline; border-bottom: 1px solid var(--color-border); padding-bottom: 8px; }
  .type-meta { display: flex; flex-direction: column; align-items: flex-end; gap: 2px; }
  .two-col { display: grid; grid-template-columns: 1fr 1fr; gap: 32px; margin-top: 12px; }
  .three-col { display: grid; grid-template-columns: 1fr 1fr 1fr; gap: 32px; margin-top: 12px; }
  .scale-row { display: flex; align-items: center; gap: 10px; min-height: 28px; }
  .scale-bar { height: 12px; border-radius: var(--radius-full); display: inline-block; }
  .density-table { display: grid; grid-template-columns: repeat(3, 1fr); gap: 16px; margin-top: 20px; border-top: 1px solid var(--color-border); padding-top: 16px; }
  .density-table > div { display: flex; flex-direction: column; gap: 4px; }
  .elev-card { height: 42px; display: grid; place-items: center; background: var(--color-surface-raised); border-radius: var(--radius-md); margin: 10px 0; }
  .focus-demo { display: inline-flex; align-items: center; height: 34px; padding: 0 12px; border-radius: var(--radius-sm); background: var(--color-primary); color: var(--color-on-primary); font-weight: 600; margin: 8px 8px 8px 0; }
  .focus-demo.hc { outline-width: 3px; }
  .motion-track { width: 360px; height: 12px; background: var(--color-surface-sunken); border-radius: var(--radius-full); overflow: hidden; }
  .motion-fill { display: block; height: 100%; border-radius: var(--radius-full); }
</style>"""
    return f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8"><title>Chemical UI token sheet</title>
<style>
{css}
{extra}
</style></head>
<body><main class="sheet">
<div class="h1" style="font-size:var(--text-display-size);line-height:var(--text-display-line);font-weight:var(--text-display-weight);letter-spacing:var(--text-display-letter);margin-bottom:4px">Chemical UI tokens</div>
<p class="muted">P0.4 token sheet · light/dark/high-contrast · compact density · exact values from tokens.json.</p>
{SHEET_MARKER}
{token_sheet(tokens)}
</main>
<script>
const p = new URLSearchParams(location.search);
if (p.get("theme")) document.documentElement.dataset.theme = p.get("theme");
if (p.get("density")) document.documentElement.dataset.density = p.get("density");
</script>
</body></html>
"""


def render_template(tokens, name):
    path = os.path.join(TEMPLATES, name)
    text = open(path, encoding="utf-8").read()
    text = text.replace(MARKER, inline_css(tokens))
    if name == "tokens.html":
        text = text.replace(SHEET_MARKER, token_sheet(tokens))
    return text


def main():
    check = "--check" in sys.argv[1:]
    unknown = [a for a in sys.argv[1:] if a != "--check"]
    if unknown:
        print("ui-gen-design: unknown argument " + " ".join(unknown), file=sys.stderr)
        return 2
    tokens = json.load(open(TOKENS, encoding="utf-8"))
    os.makedirs(DESIGN, exist_ok=True)
    stale = []
    for template, output in FILES.items():
        if template == "tokens.html":
            text = token_sheet_page(tokens)
        else:
            text = render_template(tokens, template)
        out_path = os.path.join(DESIGN, output)
        if check:
            if not os.path.isfile(out_path) or open(out_path, encoding="utf-8").read() != text:
                stale.append(output)
        else:
            with open(out_path, "w", encoding="utf-8") as f:
                f.write(text)
    if check:
        if stale:
            print("ui-gen-design: stale generated references: " + ", ".join(stale), file=sys.stderr)
            return 1
        print("ui-gen-design: references match tokens.json")
        return 0
    print("ui-gen-design: wrote " + ", ".join(FILES.values()))
    return 0


if __name__ == "__main__":
    sys.exit(main())
