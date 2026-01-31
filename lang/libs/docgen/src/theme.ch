public namespace docgen {

public func get_default_css() : std::string_view {
    return std::string_view(""":root {
    --bg-color: #ffffff;
    --text-color: #24292e;
    --sidebar-bg: #f6f8fa;
    --border-color: #e1e4e8;
    --code-bg: #f6f8fa;
    --link-color: #0366d6;
    --blockquote-color: #6a737d;
}
[data-theme="dark"] {
    --bg-color: #0d1117;
    --text-color: #c9d1d9;
    --sidebar-bg: #161b22;
    --border-color: #30363d;
    --code-bg: #161b22;
    --link-color: #58a6ff;
    --blockquote-color: #8b949e;
}
body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Helvetica, Arial, sans-serif; background: var(--bg-color); color: var(--text-color); margin: 0; display: flex; height: 100vh; }
a { color: var(--link-color); text-decoration: none; }
a:hover { text-decoration: underline; }
#sidebar { width: 300px; background: var(--sidebar-bg); border-right: 1px solid var(--border-color); overflow-y: auto; padding: 20px; display: flex; flex-direction: column; }
#content { flex: 1; overflow-y: auto; padding: 40px; max-width: 900px; margin: 0 auto; }
.sidebar-title { font-size: 1.2em; font-weight: bold; margin-bottom: 20px; }
.sidebar-list { list-style: none; padding: 0; margin: 0; }
.sidebar-list ul { list-style: none; padding-left: 20px; }
.sidebar-item { margin: 5px 0; }
.theme-toggle { cursor: pointer; padding: 5px; margin-bottom: 20px; background: var(--bg-color); border: 1px solid var(--border-color); color: var(--text-color); }
code { background: var(--code-bg); padding: 0.2em 0.4em; border-radius: 3px; font-family: Consolas, 'Courier New', monospace; }
pre { background: var(--code-bg); padding: 16px; overflow: auto; border-radius: 6px; }
blockquote { border-left: 4px solid var(--border-color); color: var(--blockquote-color); padding: 0 1em; margin: 0; }
table { border-collapse: collapse; width: 100%; }
th, td { border: 1px solid var(--border-color); padding: 6px 13px; }
tr:nth-child(2n) { background-color: var(--code-bg); }
img { max-width: 100%; }
""");
}

public func get_default_js() : std::string_view {
    return std::string_view("""
document.addEventListener('DOMContentLoaded', () => {
    const toggle = document.createElement('button');
    toggle.className = 'theme-toggle';
    toggle.textContent = 'Toggle Theme';
    toggle.onclick = () => {
        const current = document.documentElement.getAttribute('data-theme');
        const next = current === 'dark' ? 'light' : 'dark';
        document.documentElement.setAttribute('data-theme', next);
        localStorage.setItem('theme', next);
    };
    document.getElementById('sidebar').insertBefore(toggle, document.getElementById('sidebar').firstChild);
    
    const saved = localStorage.getItem('theme');
    if (saved) {
        document.documentElement.setAttribute('data-theme', saved);
    } else if (window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches) {
        document.documentElement.setAttribute('data-theme', 'dark');
    }
});
""");
}

}
