public func (page : &mut HtmlPage) injectDefaultComponentsTheme() {
    page.append_css_view("""
        /* 1. Base Geometry & Shared Tokens */
        .chx-default {
          --chx-font: 'Space Grotesk', system-ui, -apple-system, sans-serif;
          --chx-radius: 16px;
          --chx-radius-sm: 10px;
          --chx-border-width: 1px;

          /* Modern Easing for transitions */
          --chx-ease: cubic-bezier(0.4, 0, 0.2, 1);
          --chx-transition: 0.25s var(--chx-ease);
          --chx-shadow-sm: 0 4px 12px rgba(0,0,0,0.08);
          --chx-shadow: 0 14px 30px rgba(0,0,0,0.12);
          --chx-shadow-lg: 0 24px 50px rgba(0,0,0,0.18);
        }

        /* 2. Light Mode: Clean, soft, and breathable */
        .chx-default.light {
          /* Brand / Primary (Deep Ink Blue) */
          --chx-primary: #0f172a;
          --chx-primary-hover: #1f2937;
          --chx-primary-fg: #ffffff;

          /* Surfaces */
          --chx-bg: #f6f7fb;
          --chx-surface: #ffffff;
          --chx-surface-2: #f1f5f9;
          --chx-border: #e2e8f0;
          --chx-border-strong: #cbd5f5;

          /* Text */
          --chx-text-main: #0f172a;
          --chx-text-muted: #64748b;

          /* Status/Accents */
          --chx-accent: #2563eb; /* Trust Blue */
          --chx-error: #ef4444;
          --chx-success: #10b981;

          /* Soft Elevation */
          --chx-ring: rgba(15, 23, 42, 0.08);
        }

        /* 3. Dark Mode: Deep navy depth, high legibility */
        .chx-default.dark {
          /* Brand / Primary (Brightened for Contrast) */
          --chx-primary: #f8fafc;
          --chx-primary-hover: #e2e8f0;
          --chx-primary-fg: #0b1120;

          /* Surfaces */
          --chx-bg: #060b16; /* Deepest Navy */
          --chx-surface: #0b1220;
          --chx-surface-2: #0f172a;
          --chx-border: #1e293b;
          --chx-border-strong: #334155;

          /* Text */
          --chx-text-main: #f1f5f9;
          --chx-text-muted: #94a3b8;

          /* Status/Accents */
          --chx-accent: #60a5fa;
          --chx-error: #f87171;
          --chx-success: #34d399;

          /* Soft Glow Elevation */
          --chx-ring: rgba(248, 250, 252, 0.15);
        }

        body.chx-default {
          background: var(--chx-bg);
          color: var(--chx-text-main);
          font-family: var(--chx-font);
        }
    """)
}

public func (page : &mut HtmlPage) injectComponentsThemeScope(selector : &std::string_view, vars : &std::string_view) {
    var css = std::string()
    css.append_view(selector)
    css.append_view(" {")
    css.append_view(vars)
    css.append_view("}")
    page.append_css_view(css.to_view())
}

public func componentsThemeScope(selector : &std::string_view, vars : &std::string_view) : std::string {
    var css = std::string()
    css.append_view(selector)
    css.append_view(" {")
    css.append_view(vars)
    css.append_view("}")
    return css
}
