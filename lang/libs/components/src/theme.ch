public func (page : &mut HtmlPage) injectDefaultComponentsTheme() {
    page.append_css_view("""
        /* 1. Base Geometry & Shared Tokens */
        .chx-default {
          --chx-font: 'Inter', system-ui, -apple-system, sans-serif;
          --chx-radius: 14px;
          --chx-radius-sm: 8px;
          --chx-border-width: 1px;

          /* Modern Easing for transitions */
          --chx-ease: cubic-bezier(0.4, 0, 0.2, 1);
          --chx-transition: 0.25s var(--chx-ease);
        }

        /* 2. Light Mode: Clean, soft, and breathable */
        .chx-default.light {
          /* Brand / Primary (Deep Ink Blue) */
          --chx-primary: #0f172a;
          --chx-primary-hover: #334155;
          --chx-primary-fg: #ffffff;

          /* Surfaces */
          --chx-bg: #f8fafc;
          --chx-surface: #ffffff;
          --chx-border: #e2e8f0;

          /* Text */
          --chx-text-main: #0f172a;
          --chx-text-muted: #64748b;

          /* Status/Accents */
          --chx-accent: #3b82f6; /* Trust Blue */
          --chx-error: #ef4444;
          --chx-success: #10b981;

          /* Soft Elevation */
          --chx-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.04), 0 4px 6px -4px rgba(0, 0, 0, 0.04);
          --chx-ring: rgba(15, 23, 42, 0.08);
        }

        /* 3. Dark Mode: Deep navy depth, high legibility */
        .chx-default.dark {
          /* Brand / Primary (Brightened for Contrast) */
          --chx-primary: #f8fafc;
          --chx-primary-hover: #e2e8f0;
          --chx-primary-fg: #0f172a;

          /* Surfaces */
          --chx-bg: #020617; /* Deepest Navy */
          --chx-surface: #0f172a;
          --chx-border: #1e293b;

          /* Text */
          --chx-text-main: #f1f5f9;
          --chx-text-muted: #94a3b8;

          /* Status/Accents */
          --chx-accent: #60a5fa;
          --chx-error: #f87171;
          --chx-success: #34d399;

          /* Soft Glow Elevation */
          --chx-shadow: 0 20px 25px -5px rgba(0, 0, 0, 0.3), 0 8px 10px -6px rgba(0, 0, 0, 0.3);
          --chx-ring: rgba(248, 250, 252, 0.15);
        }
    """)
}
