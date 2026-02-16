public namespace docgen {

public func get_default_css() : std::string_view {
    return std::string_view(""":root {
    /* Default: Midnight Theme (Cyberpunk/Dev) */
    --bg-primary: #030712;
    --bg-secondary: #0f172a;
    --bg-tertiary: #1e293b;
    --text-primary: #f8fafc;
    --text-secondary: #94a3b8;
    --text-muted: #64748b;
    --accent: #38bdf8;
    --accent-hover: #0ea5e9;
    --accent-glow: rgba(56, 189, 248, 0.1);
    --border: #1e293b;
    --code-bg: #0b1221;
    --success: #22c55e;
    --warning: #f59e0b;
    --error: #ef4444;
    --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.3);
    --radius: 6px;
    --transition: 0.15s ease;
    --font-sans: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    --font-mono: 'JetBrains Mono', 'Fira Code', Consolas, monospace;
    --header-height: 64px;
    --sidebar-width: 280px;
    --content-max-width: 900px;
    
    /* Visual Params */
    --bg-gradient: linear-gradient(to bottom, #0f172a, #030712);
    --glass-blur: 0px;
    --header-bg: rgba(15, 23, 42, 0.8);
    --card-border: 1px solid var(--border);

    /* Alert Colors */
    --alert-note: #38bdf8;
    --alert-note-bg: rgba(56, 189, 248, 0.08);
    --alert-tip: #22c55e;
    --alert-tip-bg: rgba(34, 197, 94, 0.08);
    --alert-important: #a78bfa;
    --alert-important-bg: rgba(167, 139, 250, 0.08);
    --alert-warning: #f59e0b;
    --alert-warning-bg: rgba(245, 158, 11, 0.08);
    --alert-caution: #f43f5e;
    --alert-caution-bg: rgba(244, 63, 94, 0.08);
    --alert-text: var(--text-secondary);
}

[data-theme="light"] {
    --bg-primary: #ffffff;
    --bg-secondary: #f8fafc;
    --bg-tertiary: #e2e8f0;
    --text-primary: #0f172a;
    --text-secondary: #475569;
    --text-muted: #94a3b8;
    --accent: #0284c7;
    --accent-hover: #0369a1;
    --accent-glow: rgba(2, 132, 199, 0.1);
    --border: #e2e8f0;
    --code-bg: #f1f5f9;
    --shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1);
    
    /* Reset Visual Params */
    --bg-gradient: none;
    --glass-blur: 0px;
    --header-bg: var(--bg-secondary);

    /* Alert Colors */
    --alert-note: #0284c7;
    --alert-note-bg: #eff8ff;
    --alert-tip: #16a34a;
    --alert-tip-bg: #f0fdf4;
    --alert-important: #7c3aed;
    --alert-important-bg: #f5f3ff;
    --alert-warning: #d97706;
    --alert-warning-bg: #fffbeb;
    --alert-caution: #e11d48;
    --alert-caution-bg: #fff1f2;
    --alert-text: var(--text-secondary);
}

[data-theme="sunset"] {
    --bg-primary: #1c1917;
    --bg-secondary: #292524;
    --bg-tertiary: #44403c;
    --text-primary: #fef3c7;
    --text-secondary: #fcd34d;
    --text-muted: #d97706;
    --accent: #f59e0b;
    --accent-hover: #d97706;
    --accent-glow: rgba(245, 158, 11, 0.15);
    --border: #44403c;
    --code-bg: #292524;
    
    /* Reset Visual Params */
    --bg-gradient: none;
    --glass-blur: 0px;
    --header-bg: var(--bg-secondary);

    /* Alert Colors */
    --alert-note: #fbbf24;
    --alert-note-bg: rgba(251, 191, 36, 0.1);
    --alert-tip: #a3e635;
    --alert-tip-bg: rgba(163, 230, 53, 0.08);
    --alert-important: #e879f9;
    --alert-important-bg: rgba(232, 121, 249, 0.08);
    --alert-warning: #fb923c;
    --alert-warning-bg: rgba(251, 146, 60, 0.1);
    --alert-caution: #fb7185;
    --alert-caution-bg: rgba(251, 113, 133, 0.08);
    --alert-text: #fef3c7;
}

/* Aurora Theme (Northern Lights) */
[data-theme="aurora"] {
    --bg-primary: #001e2b;
    --bg-secondary: #002b3d;
    --bg-tertiary: #064e66;
    --text-primary: #e0faff;
    --text-secondary: #8ecbd6;
    --text-muted: #4a7c87;
    --accent: #00d4aa;
    --accent-hover: #00f0c0;
    --accent-glow: rgba(0, 212, 170, 0.15);
    --border: #064e66;
    --code-bg: #00151f;
    --shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.2);
    --radius: 8px;
    --header-height: 64px;
    --bg-gradient: radial-gradient(circle at top right, #00364d 0%, #001e2b 100%);
    --glass-blur: 0px;
    --header-bg: rgba(0, 30, 43, 0.9);

    /* Alert Colors */
    --alert-note: #22d3ee;
    --alert-note-bg: rgba(34, 211, 238, 0.08);
    --alert-tip: #34d399;
    --alert-tip-bg: rgba(52, 211, 153, 0.08);
    --alert-important: #a78bfa;
    --alert-important-bg: rgba(167, 139, 250, 0.08);
    --alert-warning: #fbbf24;
    --alert-warning-bg: rgba(251, 191, 36, 0.08);
    --alert-caution: #fb7185;
    --alert-caution-bg: rgba(251, 113, 133, 0.08);
    --alert-text: #8ecbd6;
}

/* Cosmic Theme (Vibrant/Glass) - Softened */
[data-theme="cosmic"] {
    --bg-primary: #13111c;
    --bg-secondary: rgba(29, 24, 46, 0.7);
    --bg-tertiary: rgba(45, 40, 70, 0.5);
    --text-primary: #e6e6f0;
    --text-secondary: #a8a8b8;
    --text-muted: #6c6c7d;
    --accent: #c4b5fd; /* Softened Purple */
    --accent-hover: #d8b4fe;
    --accent-glow: rgba(196, 181, 253, 0.15);
    --border: rgba(255, 255, 255, 0.08);
    --code-bg: rgba(0, 0, 0, 0.25);
    --shadow: 0 8px 32px 0 rgba(0, 0, 0, 0.2);
    --radius: 12px;
    --header-height: 70px;
    --bg-gradient: linear-gradient(180deg, #1e1b2e 0%, #13111c 100%); /* Much subtler */
    --glass-blur: 12px;
    --header-bg: rgba(19, 17, 28, 0.6);

    /* Alert Colors */
    --alert-note: #93c5fd;
    --alert-note-bg: rgba(147, 197, 253, 0.07);
    --alert-tip: #6ee7b7;
    --alert-tip-bg: rgba(110, 231, 183, 0.07);
    --alert-important: #d8b4fe;
    --alert-important-bg: rgba(216, 180, 254, 0.07);
    --alert-warning: #fcd34d;
    --alert-warning-bg: rgba(252, 211, 77, 0.07);
    --alert-caution: #fda4af;
    --alert-caution-bg: rgba(253, 164, 175, 0.07);
    --alert-text: #a8a8b8;
}

/* Minimal Theme (Dark Modern) */
[data-theme="minimal"] {
    --bg-primary: #0a0a0a;
    --bg-secondary: #000000;
    --bg-tertiary: #171717;
    --text-primary: #ededed;
    --text-secondary: #a1a1a1;
    --text-muted: #666666;
    --accent: #ffffff;
    --accent-hover: #e5e5e5;
    --accent-glow: rgba(255, 255, 255, 0.1);
    --border: #333333;
    --code-bg: #111111;
    --shadow: 0 0 0 1px #333;
    --radius: 6px;
    --header-height: 64px;
    --font-sans: 'Inter', -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    --content-max-width: 900px;
    
    /* Reset Visual Params */
    --bg-gradient: none;
    --glass-blur: 0px;
    --header-bg: rgba(0, 0, 0, 0.8); /* Slight transparency for minimal */

    /* Alert Colors - monochrome with subtle tints */
    --alert-note: #e5e5e5;
    --alert-note-bg: #111111;
    --alert-tip: #a1a1a1;
    --alert-tip-bg: #111111;
    --alert-important: #d4d4d4;
    --alert-important-bg: #111111;
    --alert-warning: #a1a1a1;
    --alert-warning-bg: #111111;
    --alert-caution: #ededed;
    --alert-caution-bg: #111111;
    --alert-text: #a1a1a1;
}

[data-theme="minimal"] .sidebar {
    background: var(--bg-secondary);
    border-right: 1px solid var(--border);
}

[data-theme="minimal"] .header {
    background: rgba(0, 0, 0, 0.7);
    border-bottom: 1px solid var(--border);
    backdrop-filter: blur(12px);
}

[data-theme="minimal"] .sidebar-item > a {
    border-radius: var(--radius);
    color: var(--text-secondary);
}

[data-theme="minimal"] .sidebar-item > a.active {
    background: var(--bg-tertiary);
    color: var(--text-primary);
    font-weight: 500;
}


* { box-sizing: border-box; margin: 0; padding: 0; }

body {
    font-family: var(--font-sans);
    background: var(--bg-primary);
    color: var(--text-primary);
    line-height: 1.7;
    min-height: 100vh;
}

/* Global Gradient Background Support */
body::before {
    content: '';
    position: fixed;
    top: 0; left: 0; right: 0; bottom: 0;
    background: var(--bg-gradient);
    z-index: -1;
}

/* Header */
.header {
    position: fixed;
    top: 0;
    left: 0;
    right: 0;
    height: var(--header-height);
    background: var(--header-bg);
    border-bottom: 1px solid var(--border);
    display: flex;
    align-items: center;
    padding: 0 24px;
    z-index: 1000;
    backdrop-filter: blur(var(--glass-blur));
}

/* Mobile Menu Button */
.menu-toggle {
    display: none;
    margin-right: 16px;
    background: transparent;
    border: none;
    color: var(--text-primary);
    cursor: pointer;
    padding: 4px;
}

.header-brand {
    font-size: 1.25rem;
    font-weight: 700;
    color: var(--accent);
    text-decoration: none;
    display: flex;
    align-items: center;
    gap: 8px;
}

.header-brand:hover { opacity: 0.9; }

.header-brand svg { width: 28px; height: 28px; }

.header-spacer { flex: 1; }

/* Search */
.search-container {
    position: relative;
    width: 320px;
    margin-right: 24px;
}

.search-input {
    width: 100%;
    padding: 10px 16px 10px 42px;
    background: var(--bg-tertiary);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-primary);
    font-size: 14px;
    transition: var(--transition);
    outline: none;
}

.search-input::placeholder { color: var(--text-muted); }

.search-input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 3px var(--accent-glow);
}

.search-icon {
    position: absolute;
    left: 14px;
    top: 50%;
    transform: translateY(-50%);
    color: var(--text-muted);
    pointer-events: none;
}

.search-kbd {
    position: absolute;
    right: 12px;
    top: 50%;
    transform: translateY(-50%);
    padding: 2px 6px;
    background: var(--bg-secondary);
    border: 1px solid var(--border);
    border-radius: 4px;
    font-size: 11px;
    color: var(--text-muted);
    font-family: monospace;
}

.search-results {
    position: absolute;
    top: 100%;
    left: 0;
    right: 0;
    margin-top: 8px;
    background: var(--bg-secondary);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    max-height: 400px;
    overflow-y: auto;
    display: none;
    z-index: 1001;
}

.search-results.open { display: block; }

.search-result-item {
    padding: 12px 16px;
    border-bottom: 1px solid var(--border);
    cursor: pointer;
    display: block;
    text-decoration: none;
}

.search-result-item:last-child { border-bottom: none; }
.search-result-item:hover { background: var(--bg-tertiary); }

.search-result-title {
    color: var(--accent);
    font-weight: 600;
    font-size: 14px;
    margin-bottom: 4px;
}

.search-result-preview {
    color: var(--text-secondary);
    font-size: 12px;
    line-height: 1.4;
    display: -webkit-box;
    -webkit-line-clamp: 2;
    -webkit-box-orient: vertical;
    overflow: hidden;
}

/* Theme Controls */
.header-controls {
    display: flex;
    align-items: center;
    gap: 12px;
}

.theme-select {
    padding: 8px 12px;
    background: var(--bg-tertiary);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    color: var(--text-primary);
    font-size: 13px;
    cursor: pointer;
    outline: none;
    transition: var(--transition);
}

.theme-select:hover { border-color: var(--accent); }

/* Layout */
.app-layout {
    display: flex;
    margin-top: var(--header-height);
    min-height: calc(100vh - var(--header-height));
}

/* Sidebar */
.sidebar {
    width: var(--sidebar-width);
    background: var(--bg-secondary);
    border-right: 1px solid var(--border);
    padding: 24px 0;
    position: fixed;
    top: var(--header-height);
    left: 0;
    bottom: 0;
    overflow-y: auto;
    transition: var(--transition);
}

::-webkit-scrollbar { width: 6px; height: 6px; }
::-webkit-scrollbar-track { background: transparent; }
::-webkit-scrollbar-thumb { background: var(--border); border-radius: 3px; }
::-webkit-scrollbar-thumb:hover { background: var(--text-muted); }

html { scroll-behavior: smooth; }

::selection {
    background: var(--accent);
    color: var(--bg-primary);
}

.sidebar-section { margin-bottom: 8px; }

.sidebar-list {
    list-style: none;
    padding: 0 16px;
}

.sidebar-list ul {
    list-style: none;
    padding-left: 0;
    margin-top: 4px;
    margin-left: 16px;
    border-left: 1px solid var(--border);
}

.sidebar-item { margin: 2px 0; }

.sidebar-item > a,
.sidebar-item > .sidebar-header {
    display: block;
    padding: 8px 12px;
    border-radius: 6px;
    transition: var(--transition);
    font-size: 14px;
}

.sidebar-item > a {
    color: var(--text-secondary);
    text-decoration: none;
}

.sidebar-item > a:hover {
    color: var(--text-primary);
    background: var(--bg-tertiary);
}

.sidebar-item > a.active {
    color: var(--accent);
    background: var(--accent-glow);
    font-weight: 500;
}

.sidebar-header {
    color: var(--text-primary);
    font-weight: 600;
    font-size: 13px;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

/* Content */
.content {
    flex: 1;
    margin-left: var(--sidebar-width);
    padding: 48px 64px;
    max-width: var(--content-max-width);
}

.content h1, .content h2, .content h3, .content h4 {
    color: var(--text-primary);
    margin-top: 2rem;
    margin-bottom: 1rem;
    font-weight: 600;
    line-height: 1.3;
}

.content h1 { font-size: 2.25rem; margin-top: 0; }
.content h2 { font-size: 1.75rem; border-bottom: 1px solid var(--border); padding-bottom: 0.5rem; }
.content h3 { font-size: 1.375rem; }
.content h4 { font-size: 1.125rem; }

.content p { margin-bottom: 1rem; color: var(--text-secondary); }

.content a {
    color: var(--accent);
    text-decoration: none;
    border-bottom: 1px solid transparent;
    transition: var(--transition);
}

.content a:hover { border-bottom-color: var(--accent); }

.content ul, .content ol {
    margin-bottom: 1rem;
    padding-left: 1.5rem;
    color: var(--text-secondary);
}

.content li { margin-bottom: 0.5rem; }

.content code {
    background: var(--code-bg);
    padding: 0.2em 0.4em;
    border-radius: 4px;
    font-family: 'JetBrains Mono', 'Fira Code', Consolas, monospace;
    font-size: 0.9em;
    color: var(--accent);
}

.content pre {
    background: var(--code-bg);
    padding: 20px;
    border-radius: var(--radius);
    overflow-x: auto;
    margin-bottom: 1rem;
    border: 1px solid var(--border);
}

.content pre code {
    background: none;
    padding: 0;
    color: var(--text-primary);
}

.content blockquote {
    border-left: 4px solid var(--accent);
    padding: 1rem 1.5rem;
    margin: 1rem 0;
    background: var(--accent-glow);
    border-radius: 0 var(--radius) var(--radius) 0;
    color: var(--text-secondary);
    transition: border-color var(--transition);
}

/* === Alerts === */
.content .md-alert {
    border-left-width: 4px;
    border-left-style: solid;
    padding: 1rem 1.25rem;
    margin: 1.5rem 0;
    border-radius: 0 var(--radius) var(--radius) 0;
    color: var(--alert-text);
    transition: background var(--transition), border-color var(--transition);
}

.content .md-alert p {
    color: var(--alert-text);
    margin-bottom: 0.5rem;
}

.content .md-alert p:last-child {
    margin-bottom: 0;
}

.content .md-alert-title {
    font-weight: 700;
    margin-bottom: 0.5rem;
    display: flex;
    align-items: center;
    gap: 8px;
    text-transform: uppercase;
    font-size: 0.8rem;
    letter-spacing: 0.08em;
}

.content .md-alert-title::before {
    content: '';
    display: inline-block;
    width: 16px;
    height: 16px;
    flex-shrink: 0;
    background-size: contain;
    background-repeat: no-repeat;
    background-position: center;
}

/* Note */
.content .md-alert-note {
    border-left-color: var(--alert-note);
    background: var(--alert-note-bg);
}
.content .md-alert-note .md-alert-title {
    color: var(--alert-note);
}
.content .md-alert-note .md-alert-title::before {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16' width='16' height='16'%3E%3Cpath fill='%2338bdf8' d='M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8Zm8-6.5a6.5 6.5 0 1 0 0 13 6.5 6.5 0 0 0 0-13ZM6.5 7.75A.75.75 0 0 1 7.25 7h1a.75.75 0 0 1 .75.75v2.75h.25a.75.75 0 0 1 0 1.5h-2a.75.75 0 0 1 0-1.5h.25v-2h-.25a.75.75 0 0 1-.75-.75ZM8 6a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z'/%3E%3C/svg%3E");
}

/* Tip */
.content .md-alert-tip {
    border-left-color: var(--alert-tip);
    background: var(--alert-tip-bg);
}
.content .md-alert-tip .md-alert-title {
    color: var(--alert-tip);
}
.content .md-alert-tip .md-alert-title::before {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16' width='16' height='16'%3E%3Cpath fill='%2322c55e' d='M8 1.5c-2.363 0-4 1.69-4 3.75 0 .984.424 1.625.984 2.304l.214.253c.223.264.47.556.673.848.284.411.537.896.621 1.49a.75.75 0 0 1-1.484.211c-.04-.282-.163-.547-.37-.847a8.456 8.456 0 0 0-.542-.68c-.084-.1-.173-.205-.268-.32C3.201 7.75 2.5 6.766 2.5 5.25 2.5 2.31 4.863 0 8 0s5.5 2.31 5.5 5.25c0 1.516-.701 2.5-1.328 3.259-.095.115-.184.22-.268.319-.207.245-.383.453-.541.681-.208.3-.33.565-.37.847a.751.751 0 0 1-1.485-.212c.084-.593.337-1.078.621-1.489.203-.292.45-.584.673-.848.075-.088.147-.173.213-.253.561-.679.985-1.32.985-2.304 0-2.06-1.637-3.75-4-3.75ZM5.75 12h4.5a.75.75 0 0 1 0 1.5h-4.5a.75.75 0 0 1 0-1.5ZM6 15.25a.75.75 0 0 1 .75-.75h2.5a.75.75 0 0 1 0 1.5h-2.5a.75.75 0 0 1-.75-.75Z'/%3E%3C/svg%3E");
}

/* Important */
.content .md-alert-important {
    border-left-color: var(--alert-important);
    background: var(--alert-important-bg);
}
.content .md-alert-important .md-alert-title {
    color: var(--alert-important);
}
.content .md-alert-important .md-alert-title::before {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16' width='16' height='16'%3E%3Cpath fill='%23a78bfa' d='M0 1.75C0 .784.784 0 1.75 0h12.5C15.216 0 16 .784 16 1.75v9.5A1.75 1.75 0 0 1 14.25 13H8.06l-2.573 2.573A1.458 1.458 0 0 1 3 14.543V13H1.75A1.75 1.75 0 0 1 0 11.25Zm1.75-.25a.25.25 0 0 0-.25.25v9.5c0 .138.112.25.25.25h2a.75.75 0 0 1 .75.75v2.19l2.72-2.72a.749.749 0 0 1 .53-.22h6.5a.25.25 0 0 0 .25-.25v-9.5a.25.25 0 0 0-.25-.25Zm7 2.25v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 9a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z'/%3E%3C/svg%3E");
}

/* Warning */
.content .md-alert-warning {
    border-left-color: var(--alert-warning);
    background: var(--alert-warning-bg);
}
.content .md-alert-warning .md-alert-title {
    color: var(--alert-warning);
}
.content .md-alert-warning .md-alert-title::before {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16' width='16' height='16'%3E%3Cpath fill='%23f59e0b' d='M6.457 1.047c.659-1.234 2.427-1.234 3.086 0l6.082 11.378A1.75 1.75 0 0 1 14.082 15H1.918a1.75 1.75 0 0 1-1.543-2.575Zm1.763.707a.25.25 0 0 0-.44 0L1.698 13.132a.25.25 0 0 0 .22.368h12.164a.25.25 0 0 0 .22-.368Zm.53 3.996v2.5a.75.75 0 0 1-1.5 0v-2.5a.75.75 0 0 1 1.5 0ZM9 11a1 1 0 1 1-2 0 1 1 0 0 1 2 0Z'/%3E%3C/svg%3E");
}

/* Caution */
.content .md-alert-caution {
    border-left-color: var(--alert-caution);
    background: var(--alert-caution-bg);
}
.content .md-alert-caution .md-alert-title {
    color: var(--alert-caution);
}
.content .md-alert-caution .md-alert-title::before {
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16' width='16' height='16'%3E%3Cpath fill='%23f43f5e' d='M4.47.22A.749.749 0 0 1 5 0h6c.199 0 .389.079.53.22l4.25 4.25c.141.14.22.331.22.53v6a.749.749 0 0 1-.22.53l-4.25 4.25A.749.749 0 0 1 11 16H5a.749.749 0 0 1-.53-.22L.22 11.53A.749.749 0 0 1 0 11V5c0-.199.079-.389.22-.53Zm.84 1.28L1.5 5.31v5.38l3.81 3.81h5.38l3.81-3.81V5.31L10.69 1.5ZM8 4a.75.75 0 0 1 .75.75v3.5a.75.75 0 0 1-1.5 0v-3.5A.75.75 0 0 1 8 4Zm0 8a1 1 0 1 1 0-2 1 1 0 0 1 0 2Z'/%3E%3C/svg%3E");
}

/* Minimal theme: bordered alerts */
[data-theme="minimal"] .content .md-alert {
    border-top: 1px solid var(--border);
    border-right: 1px solid var(--border);
    border-bottom: 1px solid var(--border);
}

.content table {
    width: 100%;
    border-collapse: collapse;
    margin-bottom: 1rem;
}

.content th, .content td {
    border: 1px solid var(--border);
    padding: 12px 16px;
    text-align: left;
}

.content th {
    background: var(--bg-tertiary);
    font-weight: 600;
}

.content tr:hover { background: var(--bg-secondary); }

.content img {
    max-width: 100%;
    border-radius: var(--radius);
    margin: 1rem 0;
}

/* Mobile Responsive */
@media (max-width: 1024px) {
    .sidebar { 
        transform: translateX(-100%); 
        z-index: 2000;
        background: var(--bg-primary); /* Ensure opaque on mobile usually, or match theme */
        width: 280px;
        box-shadow: 2px 0 12px rgba(0,0,0,0.5);
    }
    .sidebar.open { transform: translateX(0); }
    .content { margin-left: 0; padding: 32px 24px; }
    .search-container { width: 200px; }
    .menu-toggle { display: block; }

    /* Mobile Backdrop */
    .sidebar-backdrop {
        position: fixed;
        top: 0; left: 0; right: 0; bottom: 0;
        background: rgba(0,0,0,0.5);
        z-index: 1999;
        display: none;
        opacity: 0;
        transition: opacity 0.3s ease;
    }
    .sidebar.open + .sidebar-backdrop, .sidebar-backdrop.open {
        display: block;
        opacity: 1;
    }
}

@media (max-width: 640px) {
    .search-container { display: none; }
    .header { padding: 0 16px; }
    .content { padding: 24px 16px; }
}

/* Animations */
@keyframes fadeIn {
    from { opacity: 0; transform: translateY(10px); }
    to { opacity: 1; transform: translateY(0); }
}

.content { animation: fadeIn 0.3s ease; }


/* Generic Syntax Highlighting */
.tok-kwd { color: #f472b6; font-weight: 600; } /* Pink-400 */
.tok-str { color: #a3e635; } /* Lime-400 */
.tok-num { color: #fb923c; } /* Orange-400 */
.tok-com { color: #64748b; font-style: italic; } /* Slate-500 */
.tok-fn  { color: #60a5fa; } /* Blue-400 */
.tok-type { color: #c084fc; } /* Purple-400 */
.tok-macro { color: #22d3ee; } /* Cyan-400 */
.tok-tag { color: #f472b6; } /* Pink-400 */
.tok-attr { color: #22d3ee; } /* Cyan-400 */
.tok-var { color: #f87171; } /* Red-400 */

[data-theme="light"] .tok-kwd { color: #d946ef; }
[data-theme="light"] .tok-str { color: #65a30d; }
[data-theme="light"] .tok-num { color: #d97706; }
[data-theme="light"] .tok-com { color: #94a3b8; }
[data-theme="light"] .tok-fn  { color: #2563eb; }
[data-theme="light"] .tok-type { color: #9333ea; }
[data-theme="light"] .tok-macro { color: #0891b2; }
[data-theme="light"] .tok-tag { color: #d946ef; }
[data-theme="light"] .tok-attr { color: #0891b2; }
[data-theme="light"] .tok-var { color: #dc2626; }
""");
}

public func get_theme_init_js() : std::string_view {
    return std::string_view("""
(function() {
    function setTheme(theme) {
        if (theme === 'default') {
            document.documentElement.removeAttribute('data-theme');
        } else {
            document.documentElement.setAttribute('data-theme', theme);
        }
        localStorage.setItem('theme', theme);
        
        // Update select if it exists (later)
        const select = document.getElementById('theme-select');
        if (select) select.value = theme;
    }
    window.setTheme = setTheme;
    
    // Immediate load
    const saved = localStorage.getItem('theme');
    if (saved) {
        setTheme(saved);
    } else if (window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches) {
        setTheme('light');
    }
})();
""");
}

public func get_default_js() : std::string_view {
    return std::string_view("""
document.addEventListener('DOMContentLoaded', () => {
    // Theme management
    const themes = ['default', 'light', 'sunset', 'minimal', 'cosmic', 'aurora'];
    const themeNames = {
        'default': 'Midnight',
        'light': 'Light',
        'sunset': 'Sunset',
        'minimal': 'Minimal',
        'cosmic': 'Cosmic',
        'aurora': 'Aurora'
    };
    
    // Create theme select
    const themeSelect = document.getElementById('theme-select');
    if (themeSelect) {
        themes.forEach(theme => {
            const opt = document.createElement('option');
            opt.value = theme;
            opt.textContent = themeNames[theme];
            themeSelect.appendChild(opt);
        });
        
        const saved = localStorage.getItem('theme') || 'default';
        themeSelect.value = saved;
        
        themeSelect.addEventListener('change', (e) => window.setTheme(e.target.value));
    }
    
    // Scroll Active Sidebar Item into View
    const activeItem = document.querySelector('.sidebar-item > a.active');
    if (activeItem) {
        activeItem.scrollIntoView({ block: 'center', behavior: 'instant' });
    }
    
    // Search Logic
    const searchInput = document.getElementById('search-input');
    const searchContainer = document.querySelector('.search-container');
    
    if (searchInput && window.searchIndex) {
        // Create results container
        const resultsBox = document.createElement('div');
        resultsBox.className = 'search-results';
        searchContainer.appendChild(resultsBox);
        
        searchInput.addEventListener('input', (e) => {
            const query = e.target.value.toLowerCase();
            if (query.length < 2) {
                resultsBox.classList.remove('open');
                return;
            }
            
            const results = window.searchIndex.filter(item => {
                return item.title.toLowerCase().includes(query) || 
                       item.content.toLowerCase().includes(query);
            }).slice(0, 5);
            
            if (results.length > 0) {
                resultsBox.innerHTML = '';
                results.forEach(res => {
                    const a = document.createElement('a');
                    // FIX: Use window.rootPath to resolve full relative path
                    const root = window.rootPath || './';
                    a.href = root + res.link;
                    
                    a.className = 'search-result-item';
                    a.innerHTML = `
                         <div class="search-result-title">${res.title}</div>
                         <div class="search-result-preview">${res.snippet}</div>
                    `;
                    resultsBox.appendChild(a);
                });
                resultsBox.classList.add('open');
            } else {
                resultsBox.classList.remove('open');
            }
        });
        
        // Close on click outside
        document.addEventListener('click', (e) => {
            if (!searchContainer.contains(e.target)) {
                resultsBox.classList.remove('open');
            }
        });
        
        // Shortcuts
        document.addEventListener('keydown', (e) => {
            if ((e.ctrlKey || e.metaKey) && e.key === 'k') {
                e.preventDefault();
                searchInput.focus();
            }
            if (e.key === 'Escape') {
                searchInput.blur();
                resultsBox.classList.remove('open');
            }
        });
    }
    
    // Mobile sidebar
    const menuBtn = document.getElementById('menu-toggle');
    const sidebar = document.querySelector('.sidebar');
    const backdrop = document.createElement('div');
    backdrop.className = 'sidebar-backdrop';
    document.body.appendChild(backdrop);

    if (menuBtn && sidebar) {
        function toggleSidebar() {
            sidebar.classList.toggle('open');
            backdrop.classList.toggle('open');
        }
        
        menuBtn.addEventListener('click', toggleSidebar);
        backdrop.addEventListener('click', toggleSidebar);
    }
});
""");
}

}
