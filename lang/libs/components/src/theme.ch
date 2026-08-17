// Shadcn-style theme for the Chemical components library.
//
// The design tokens follow shadcn/ui's zinc theme exactly: CSS custom
// properties defined as HSL triplets (consumed via hsl(var(--...))) with a
// `.dark` class for dark mode. No legacy --chx-* aliases are kept.
//
// Usage:
//   page.injectDefaultComponentsTheme()   // :root + .dark tokens
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
            --radius-sm: 0.375rem;
            --radius-md: 0.625rem;
            --radius-lg: 0.75rem;
            --radius-xl: 1rem;
            --shadow-sm: 0 1px 2px 0 rgb(0 0 0 / 0.05);
            --shadow: 0 1px 3px 0 rgb(0 0 0 / 0.1), 0 1px 2px -1px rgb(0 0 0 / 0.1);
            --shadow-md: 0 4px 6px -1px rgb(0 0 0 / 0.1), 0 2px 4px -2px rgb(0 0 0 / 0.1);
            --shadow-lg: 0 10px 15px -3px rgb(0 0 0 / 0.1), 0 4px 6px -4px rgb(0 0 0 / 0.1);
            --font-sans: 'Space Grotesk', system-ui, -apple-system, sans-serif;
            --font-mono: ui-monospace, 'SFMono-Regular', 'Menlo', 'Consolas', monospace;
            --ease: cubic-bezier(0.4, 0, 0.2, 1);
            --transition: 0.25s var(--ease);
            --border-width: 1px;
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

        body.chx-default {
            background: hsl(var(--background));
            color: hsl(var(--foreground));
            font-family: var(--font-sans);
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
        @keyframes chx-toast-in {
            from { opacity: 0; transform: translateY(0.75rem); }
            to { opacity: 1; transform: translateY(0); }
        }
        @keyframes chx-toast-out {
            from { opacity: 1; transform: translateY(0); }
            to { opacity: 0; transform: translateY(0.75rem); }
        }
        @keyframes chx-sheet-in-right {
            from { transform: translateX(100%); }
            to { transform: translateX(0); }
        }
        @keyframes chx-sheet-in-left {
            from { transform: translateX(-100%); }
            to { transform: translateX(0); }
        }
        @keyframes chx-sheet-in-top {
            from { transform: translateY(-100%); }
            to { transform: translateY(0); }
        }
        @keyframes chx-sheet-in-bottom {
            from { transform: translateY(100%); }
            to { transform: translateY(0); }
        }
        @keyframes chx-slide-down {
            from { opacity: 0; transform: translateY(-0.5rem); }
            to { opacity: 1; transform: translateY(0); }
        }
        @keyframes chx-collapsible-down {
            from { height: 0; opacity: 0; }
            to { height: var(--chx-collapsible-height, 20rem); opacity: 1; }
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
