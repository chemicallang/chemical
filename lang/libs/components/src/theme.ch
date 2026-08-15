// Shadcn-style theme for the Chemical components library.
//
// The design tokens follow shadcn/ui's zinc theme exactly: CSS custom
// properties defined as HSL triplets (consumed via hsl(var(--...))) with a
// `.dark` class for dark mode. Legacy `--chx-*` variables are kept as aliases
// so existing page CSS keeps working unchanged.
//
// Usage:
//   page.injectDefaultComponentsTheme()   // :root + .dark + --chx-* aliases
//   <html class="dark">                   // opt into dark mode
public func (page : &mut HtmlPage) injectDefaultComponentsTheme() {
    page.append_css_view("""
        /* ============ Shadcn zinc theme ============ */
        :root {
            --background: 0 0% 100%;
            --foreground: 240 10% 3.9%;
            --card: 0 0% 100%;
            --card-foreground: 240 10% 3.9%;
            --popover: 0 0% 100%;
            --popover-foreground: 240 10% 3.9%;
            --primary: 240 5.9% 10%;
            --primary-foreground: 0 0% 98%;
            --secondary: 240 4.8% 95.9%;
            --secondary-foreground: 240 5.9% 10%;
            --muted: 240 4.8% 95.9%;
            --muted-foreground: 240 3.8% 46.1%;
            --accent: 240 4.8% 95.9%;
            --accent-foreground: 240 5.9% 10%;
            --destructive: 0 84.2% 60.2%;
            --destructive-foreground: 0 0% 98%;
            --success: 142.1 76.2% 36.3%;
            --success-foreground: 355.7 100% 97.3%;
            --warning: 38 92% 50%;
            --warning-foreground: 48 96% 89%;
            --info: 221.2 83.2% 53.3%;
            --info-foreground: 210 40% 98%;
            --border: 240 5.9% 90%;
            --input: 240 5.9% 90%;
            --ring: 240 5.9% 10%;
            --radius: 0.5rem;
            --shadow-sm: 0 1px 2px 0 rgb(0 0 0 / 0.05);
            --shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1), 0 1px 2px -1px rgb(0 0 0 / 0.1);
            --shadow-md: 0 4px 6px -1px rgb(0 0 0 / 0.1), 0 2px 4px -2px rgb(0 0 0 / 0.1);
            --shadow-lg: 0 10px 15px -3px rgb(0 0 0 / 0.1), 0 4px 6px -4px rgb(0 0 0 / 0.1);
        }

        .dark {
            --background: 240 10% 3.9%;
            --foreground: 0 0% 98%;
            --card: 240 10% 3.9%;
            --card-foreground: 0 0% 98%;
            --popover: 240 10% 3.9%;
            --popover-foreground: 0 0% 98%;
            --primary: 0 0% 98%;
            --primary-foreground: 240 5.9% 10%;
            --secondary: 240 3.7% 15.9%;
            --secondary-foreground: 0 0% 98%;
            --muted: 240 3.7% 15.9%;
            --muted-foreground: 240 5% 64.9%;
            --accent: 240 3.7% 15.9%;
            --accent-foreground: 0 0% 98%;
            --destructive: 0 62.8% 30.6%;
            --destructive-foreground: 0 0% 98%;
            --success: 142.1 70.6% 45.3%;
            --success-foreground: 144.9 80.4% 10%;
            --warning: 38 92% 50%;
            --warning-foreground: 48 96% 89%;
            --info: 217.2 91.2% 59.8%;
            --info-foreground: 210 40% 98%;
            --border: 240 3.7% 15.9%;
            --input: 240 3.7% 15.9%;
            --ring: 240 4.9% 83.9%;
            --shadow-sm: 0 1px 2px 0 rgb(0 0 0 / 0.4);
            --shadow: 0 1px 3px 0 rgb(0 0 0 / 0.4), 0 1px 2px -1px rgb(0 0 0 / 0.4);
            --shadow-md: 0 4px 6px -1px rgb(0 0 0 / 0.4), 0 2px 4px -2px rgb(0 0 0 / 0.4);
            --shadow-lg: 0 10px 15px -3px rgb(0 0 0 / 0.4), 0 4px 6px -4px rgb(0 0 0 / 0.4);
        }

        /* ============ Legacy --chx-* aliases ============ */
        :root, .dark {
            --chx-font: 'Space Grotesk', system-ui, -apple-system, sans-serif;
            --chx-ease: cubic-bezier(0.4, 0, 0.2, 1);
            --chx-transition: 0.25s var(--chx-ease);
            --chx-radius: 0.5rem;
            --chx-radius-sm: 0.375rem;
            --chx-border-width: 1px;
            --chx-shadow-sm: 0 1px 2px 0 rgb(0 0 0 / 0.05);
            --chx-shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1), 0 1px 2px -1px rgb(0 0 0 / 0.1);
            --chx-shadow-lg: 0 10px 15px -3px rgb(0 0 0 / 0.1), 0 4px 6px -4px rgb(0 0 0 / 0.1);
        }

        :root {
            --chx-bg: #ffffff;
            --chx-surface: #ffffff;
            --chx-surface-2: #f4f4f5;
            --chx-primary: #18181b;
            --chx-primary-hover: #27272a;
            --chx-primary-fg: #fafafa;
            --chx-text-main: #09090b;
            --chx-text-muted: #71717a;
            --chx-border: #e4e4e7;
            --chx-border-strong: #d4d4d8;
            --chx-accent: #2563eb;
            --chx-error: #dc2626;
            --chx-success: #16a34a;
            --chx-warning: #f59e0b;
            --chx-info: #2563eb;
            --chx-ring: rgba(24, 24, 27, 0.12);
        }

        .dark {
            --chx-bg: #09090b;
            --chx-surface: #09090b;
            --chx-surface-2: #18181b;
            --chx-primary: #fafafa;
            --chx-primary-hover: #e4e4e7;
            --chx-primary-fg: #18181b;
            --chx-text-main: #fafafa;
            --chx-text-muted: #a1a1aa;
            --chx-border: #27272a;
            --chx-border-strong: #3f3f46;
            --chx-accent: #3b82f6;
            --chx-error: #ef4444;
            --chx-success: #22c55e;
            --chx-warning: #f59e0b;
            --chx-info: #3b82f6;
            --chx-ring: rgba(250, 250, 250, 0.18);
        }

        body.chx-default {
            background: var(--chx-bg);
            color: var(--chx-text-main);
            font-family: var(--chx-font);
            -webkit-font-smoothing: antialiased;
        }

        /* Semantic shadcn helper classes */
        .chx-bg { background-color: hsl(var(--background)); }
        .chx-text { color: hsl(var(--foreground)); }
        .chx-text-muted-c { color: hsl(var(--muted-foreground)); }
        .chx-border-c { border-color: hsl(var(--border)); }

        /* Shared keyframes used by components */
        @keyframes chx-skeleton-shimmer {
            0% { background-position: -400px 0; }
            100% { background-position: 400px 0; }
        }
        @keyframes chx-spinner-rotate {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        @keyframes chx-fade-in {
            from { opacity: 0; }
            to { opacity: 1; }
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
