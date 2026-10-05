// The in-page JavaScript harness for #universal_test. It is appended to the
// page's HEAD script block (see runner.ch `ut_execute`), which `toString` emits
// as its own `<script>` before the body bundle that carries the fixtures' own
// `__ut_register` calls. That separation is load-bearing: a script that fails to
// PARSE is skipped in its entirety, so a harness sharing the fixture bundle's
// block would be lost with it and the host would block forever. Keep every byte
// ASCII (the page JS buffer is scanned with signed chars elsewhere).

public const UT_HARNESS : *char = """
(function(){
'use strict';
window.__ut_tests = {};
window.__ut_errors = [];
// Page-level errors, never cleared. A script that fails to parse reports a
// SyntaxError to the window and is then skipped entirely, so every statement
// in that block -- including the fixtures' `__ut_register` calls -- is lost.
// Recording them here is what turns that silent breakage into a reported
// failure instead of a hang.
window.__ut_page_errors = [];
// The names the host expects to run, injected by the runner just before this
// harness. Lets us tell "this test failed" apart from "this test never got to
// run because the bundle that registers it did not parse".
if(!window.__ut_expected) { window.__ut_expected = []; }
window.__ut_scope = null;
window.__ut_register = function(name, isolate, fn) { window.__ut_tests[name] = fn; };
window.addEventListener('error', function(e){
    var m = '' + (e && e.message ? e.message : e);
    // Keep the source position: it is the only pointer to the offending byte in
    // the emitted bundle.
    if(e && e.filename) { m += ' (' + e.filename + ':' + (e.lineno || 0) + ':' + (e.colno || 0) + ')'; }
    if(window.__ut_page_errors) window.__ut_page_errors.push(m);
    if(window.__ut_errors) window.__ut_errors.push(m);
});
window.addEventListener('unhandledrejection', function(e){ window.__ut_errors.push('' + (e && e.reason ? e.reason : e)); });
// Surface console.error/warn emitted during a test (hydration failures, dispatch
// misses, disposed-signal writes, ...) alongside an actual assertion failure.
// These are diagnostics only: many tests deliberately exercise error paths that
// log via console, so console output must never fail a test on its own.
window.__ut_console = [];
// A SECOND log that is NEVER cleared. `__ut_console` is reset per test, which is
// right for "what did this test log" and useless for anything that happens at
// PAGE LOAD: hydration runs before the first test body executes, so a test
// asserting on hydration saw an empty array and passed while the page was
// flooding the real console. `__ut_console_all` keeps the load-time output so a
// test can assert on it.
window.__ut_console_all = [];
(function(){
    var origError = console.error, origWarn = console.warn;
    var fmt = function(a){ try { return '' + (a && a.message ? a.message : a); } catch(e){ return '' + a; } };
    var record = function(kind, m, args){
        var line = kind + ': ' + m;
        // Keep the EXTRA arguments, quoted so whitespace is visible. A hydration
        // warning's payload is its expected/got pair, and without them the log says
        // only THAT something differed - which is the one thing the author already
        // knows.
        for (var i = 1; args && i < args.length; i++) {
            var extra;
            try { extra = JSON.stringify(args[i]); } catch(e) { extra = '' + args[i]; }
            if (extra === undefined) extra = 'undefined';
            line += ' [' + extra + ']';
        }
        if(window.__ut_console) window.__ut_console.push(line);
        if(window.__ut_console_all) window.__ut_console_all.push(line);
        return line;
    };
    console.error = function(){ record('console.error', fmt(arguments[0]), arguments); return origError.apply(console, arguments); };
    console.warn = function(){ record('console.warn', fmt(arguments[0]), arguments); return origWarn.apply(console, arguments); };
})();

function U(query) { this.el = query; this.desc = ''; }
function queryIn(sel) {
    if(window.__ut_scope) {
        var r = window.__ut_scope.querySelector(sel);
        if(r) return r;
    }
    return document.querySelector(sel);
}
function allIn(sel) {
    if(window.__ut_scope) {
        var r = window.__ut_scope.querySelectorAll(sel);
        if(r.length) return r;
    }
    return document.querySelectorAll(sel);
}
function el(dom, desc) { var u = new U(dom); u.desc = desc || ''; return u; }

U.prototype._need = function() {
    if(!this.el) throw new Error('element not found: ' + this.desc);
    return this.el;
};
U.prototype.text = function() { return this._need().textContent; };
U.prototype.attr = function(n) { return this._need().getAttribute(n); };
U.prototype.value = function() { return this._need().value; };
U.prototype.exists = function() { return !!this.el; };
U.prototype.count = function() { return this.el && this.el.length != null ? this.el.length : (this.el ? 1 : 0); };
U.prototype.isVisible = function() {
    if(!this.el) return false;
    if(!this.el.isConnected) return false;
    if(this.el.hidden) return false;
    // Walk ancestors: a hidden ancestor (display:none / visibility:hidden /
    // opacity:0) hides the element too (Playwright semantics).
    var n = this.el;
    while(n && n.nodeType === 1) {
        if(n.hidden) return false;
        var st = window.getComputedStyle(n);
        if(st.display === 'none' || st.visibility === 'hidden' || st.opacity === '0') return false;
        n = n.parentElement;
    }
    return true;
};
U.prototype.click = function() { this._need().click(); };
// Ancestor that hides `el` (display:none / visibility:hidden / opacity:0), or null.
function visibilityBlocker(el) {
    var n = el;
    while(n && n.nodeType === 1) {
        if(n.hidden) return n;
        var st = window.getComputedStyle(n);
        if(st.display === 'none' || st.visibility === 'hidden' || st.opacity === '0') return n;
        n = n.parentElement;
    }
    return null;
}
U.prototype._visInfo = function() {
    if(!this.el) return 'element not found (' + this.desc + ')';
    if(!this.el.isConnected) return 'element detached (' + this.el.tagName + ')';
    var b = visibilityBlocker(this.el);
    var own = 'tag=' + this.el.tagName + ' style="' + (this.el.getAttribute('style') || '') + '"';
    if(b === this.el) return own;
    if(b) return own + ' hiddenBy=<' + b.tagName + ' data-ut=' + (b.getAttribute('data-ut') || '') + ' style="' + (b.getAttribute('style') || '') + '">';
    return own;
};
function checkVisible(neg, actual) {
    var vis = !!(actual && actual.isVisible());
    if(neg ? vis : !vis) {
        throw new Error((neg ? 'not: ' : '') + 'expected element to be visible' + (neg ? '' : ' [' + (actual && actual._visInfo ? actual._visInfo() : 'no element') + ']'));
    }
}
function checkHidden(neg, actual) {
    var hidden = !(actual && actual.isVisible());
    if(neg ? hidden : !hidden) {
        throw new Error((neg ? 'not: ' : '') + 'expected element to be hidden');
    }
}
U.prototype.dblclick = function() { var e = this._need(); e.dispatchEvent(new MouseEvent('dblclick', { bubbles: true })); };
U.prototype.type = function(v) {
    var e = this._need();
    e.focus();
    e.value = v;
    e.dispatchEvent(new Event('input', { bubbles: true }));
    e.dispatchEvent(new Event('change', { bubbles: true }));
};
U.prototype.fill = function(v) { this.type(v); };
U.prototype.press = function(k) {
    var e = this._need();
    e.dispatchEvent(new KeyboardEvent('keydown', { key: k, bubbles: true }));
    e.dispatchEvent(new KeyboardEvent('keyup', { key: k, bubbles: true }));
};
U.prototype.hover = function() {
    var e = this._need();
    e.dispatchEvent(new MouseEvent('mouseover', { bubbles: true }));
    e.dispatchEvent(new MouseEvent('mouseenter', { bubbles: true }));
};
U.prototype.focus = function() { this._need().focus(); };
U.prototype.blur = function() { this._need().blur(); };
U.prototype.check = function() { var e = this._need(); if(!e.checked) e.click(); };
U.prototype.uncheck = function() { var e = this._need(); if(e.checked) e.click(); };
U.prototype.selectOption = function(v) { var e = this._need(); e.value = v; e.dispatchEvent(new Event('change', { bubbles: true })); };
U.prototype.scrollIntoView = function() { this._need().scrollIntoView(); };
U.prototype.nth = function(i) { return el(this.el && this.el.length != null ? this.el[i] : (i === 0 ? this.el : null), this.desc); };
U.prototype.first = function() { return this.nth(0); };
U.prototype.last = function() { return el(this.el && this.el.length != null ? this.el[this.el.length - 1] : this.el, this.desc); };
U.prototype.isDisabled = function() { return !!(this.el && (this.el.disabled === true || this.el.getAttribute('aria-disabled') === 'true')); };
U.prototype.isEnabled = function() { return !this.isDisabled(); };
U.prototype.isChecked = function() { return !!(this.el && this.el.checked); };
U.prototype.isFocused = function() { return !!(this.el && document.activeElement === this.el); };
U.prototype.hasClass = function(c) { return !!(this.el && this.el.classList && this.el.classList.contains(c)); };
U.prototype.css = function(prop) { if(!this.el) return null; return window.getComputedStyle(this.el).getPropertyValue(prop); };
U.prototype.jsProp = function(name) { return this.el ? this.el[name] : undefined; };
U.prototype.setValue = function(v) { var e = this._need(); e.value = v; e.dispatchEvent(new Event('input', { bubbles: true })); e.dispatchEvent(new Event('change', { bubbles: true })); };
U.prototype.find = function(sel) { return el(this.el ? this.el.querySelector(sel) : null, this.desc + ' ' + sel); };
U.prototype.findAll = function(sel) { return new UC(this.el ? this.el.querySelectorAll(sel) : [], this.desc + ' ' + sel); };
U.prototype.containsText = function(s) { return ((this.text() || '').indexOf(s) >= 0); };
U.prototype.locator = function(sel) { return this.find(sel); };
U.prototype.getByTestId = function(id) { return el(this.el ? this.el.querySelector('[data-testid="' + id + '"]') : null, this.desc + ' testid=' + id); };
U.prototype.getByTestIdAll = function(id) { return new UC(this.el ? this.el.querySelectorAll('[data-testid="' + id + '"]') : [], this.desc + ' testid=' + id); };
U.prototype.getByText = function(t) {
    if(!this.el) return el(null, 'text=' + t);
    var all = this.el.querySelectorAll('*');
    for(var i = 0; i < all.length; i++) {
        if(all[i].children.length === 0 && all[i].textContent === t) return el(all[i], 'text=' + t);
    }
    return el(null, 'text=' + t);
};
U.prototype.getByRole = function(role, opts) {
    if(!this.el) return el(null, 'role=' + role);
    var name = opts && opts.name != null ? opts.name : null;
    var root = this.el;
    var all = this.el.querySelectorAll(roleSelector(role));
    if(name == null) return all.length ? el(all[0], 'role=' + role) : el(null, 'role=' + role);
    for(var i = 0; i < all.length; i++) { if(accessibleName(all[i], root) === name) return el(all[i], 'role=' + role + ' name=' + name); }
    for(var j = 0; j < all.length; j++) { if(accessibleName(all[j], root).indexOf(name) >= 0) return el(all[j], 'role=' + role + ' name=' + name); }
    return el(null, 'role=' + role + ' name=' + name + ' candidates=[' + nameList(all, root) + ']');
};
function nameList(list, root) {
    var out = [];
    for(var i = 0; i < list.length && i < 8; i++) out.push(accessibleName(list[i], root));
    return out.join('|');
}
U.prototype.getByRoleAll = function(role, opts) {
    if(!this.el) return new UC([], 'role=' + role);
    var name = opts && opts.name != null ? opts.name : null;
    var root = this.el;
    var all = this.el.querySelectorAll(roleSelector(role));
    var out = [];
    for(var i = 0; i < all.length; i++) {
        if(name == null || accessibleName(all[i], root) === name) out.push(all[i]);
    }
    return new UC(out, 'role=' + role);
};

// A collection handle ($$ / byTestIdAll). count()/nth()/first()/last()/filter().
function UC(list, desc) { this.list = list; this.desc = desc || ''; }
UC.prototype.count = function() { return this.list.length; };
UC.prototype.nth = function(i) { return el(this.list[i], this.desc + ' nth=' + i); };
UC.prototype.first = function() { return this.nth(0); };
UC.prototype.last = function() { return this.nth(this.list.length - 1); };
UC.prototype.text = function() { return this.list.length ? this.list[0].textContent : null; };
UC.prototype.allText = function() { var a = []; for(var i = 0; i < this.list.length; i++) a.push(this.list[i].textContent); return a; };
UC.prototype.isVisible = function() { return this.list.length > 0 && el(this.list[0]).isVisible(); };
UC.prototype.click = function() { if(this.list.length) this.list[0].click(); };
UC.prototype.filter = function(opts) {
    var out = [];
    for(var i = 0; i < this.list.length; i++) {
        if(opts && opts.hasText != null) {
            if(this.list[i].textContent && this.list[i].textContent.indexOf(opts.hasText) >= 0) out.push(this.list[i]);
        } else { out.push(this.list[i]); }
    }
    return new UC(out, this.desc);
};

var UT_ROLE_TAGS = {
    button : 'button',
    link : 'a',
    heading : 'h1,h2,h3,h4,h5,h6',
    paragraph : 'p',
    textbox : 'input:not([type]),input[type=text],input[type=email],input[type=password],input[type=search],input[type=number],input[type=tel],input[type=url],textarea',
    checkbox : 'input[type=checkbox]',
    radio : 'input[type=radio]',
    combobox : 'select',
    list : 'ul,ol',
    listitem : 'li',
    table : 'table',
    img : 'img',
    slider : 'input[type=range]',
    separator : '[role=separator]',
    alert : '[role=alert]',
    dialog : '[role=dialog]',
    tablist : '[role=tablist]',
    tab : '[role=tab]',
    tabpanel : '[role=tabpanel]',
    listbox : '[role=listbox]',
    option : '[role=option]',
    switch : '[role=switch]',
    status : '[role=status]',
    menu : '[role=menu]',
    menuitem : '[role=menuitem]',
    group : '[role=group]',
    radiogroup : '[role=radiogroup]',
    tooltip : '[role=tooltip]'
};

window.$ = function(sel) { return el(queryIn(sel), sel); };
window.byCss = function(sel) { return el(queryIn(sel), sel); };
window.$$ = function(sel) { return new UC(allIn(sel), sel); };
window.byTestId = function(id) { return el(queryIn('[data-testid="' + id + '"]'), 'testid=' + id); };
window.byTestIdAll = function(id) { return new UC(allIn('[data-testid="' + id + '"]'), 'testid=' + id); };
window.byLabel = function(lbl) { return el(queryIn('[aria-label="' + lbl + '"]'), 'label=' + lbl); };
window.byText = function(t) {
    var all = allIn('*');
    for(var i = 0; i < all.length; i++) {
        if(all[i].children.length === 0 && all[i].textContent === t) return el(all[i], 'text=' + t);
    }
    return el(null, 'text=' + t);
};
function roleSelector(role) {
    var sel = '[role="' + role + '"]';
    var tags = UT_ROLE_TAGS[role];
    if(tags) sel += ',' + tags;
    return sel;
}
function accessibleName(node, root) {
    var direct = node.getAttribute('aria-label') || node.getAttribute('title');
    if(direct) return direct.trim();
    if(node.labels && node.labels.length) {
        var ls = [];
        for(var li = 0; li < node.labels.length; li++) ls.push(node.labels[li].textContent || '');
        var lj = ls.join(' ').trim();
        if(lj) return lj;
    }
    var labelledby = node.getAttribute('aria-labelledby');
    if(labelledby) {
        var parts = [];
        var ids = labelledby.split(' ');
        for(var i = 0; i < ids.length; i++) {
            var ref = labelledRef(node, ids[i], root);
            if(ref) parts.push(ref.textContent || '');
        }
        var joined = parts.join(' ').trim();
        if(joined) return joined;
    }
    return (node.textContent || '').trim();
}
// Resolves an `aria-labelledby` idref. Component libraries reuse a constant
// default id across instances (e.g. every `<Tabs>` without an `id` prop emits
// `id="tabs-tab-0"`), so a document lookup can return a *different* instance's
// element and give the wrong accessible name. A scoped locator must resolve the
// reference within its own query root first, then its test scope, then fall back
// to the document (Playwright's document-wide ARIA semantics).
function labelledRef(node, id, root) {
    var sel = '[id="' + id + '"]';
    if(root && root.querySelector) {
        var own = root.querySelector(sel);
        if(own) return own;
    }
    var scope = node && node.closest ? node.closest('[data-ut]') : null;
    if(scope) {
        var local = scope.querySelector(sel);
        if(local) return local;
    }
    if(window.__ut_scope) {
        var scoped = window.__ut_scope.querySelector(sel);
        if(scoped) return scoped;
    }
    return document.getElementById(id);
}
window.byRole = function(role, opts) {
    var name = opts && opts.name != null ? opts.name : null;
    var all = allIn(roleSelector(role));
    if(name == null) return all.length ? el(all[0], 'role=' + role) : el(null, 'role=' + role);
    for(var i = 0; i < all.length; i++) { if(accessibleName(all[i], null) === name) return el(all[i], 'role=' + role + ' name=' + name); }
    for(var j = 0; j < all.length; j++) { if(accessibleName(all[j], null).indexOf(name) >= 0) return el(all[j], 'role=' + role + ' name=' + name); }
    return el(null, 'role=' + role + ' name=' + name);
};
window.byRoleAll = function(role, opts) {
    var name = opts && opts.name != null ? opts.name : null;
    var all = allIn(roleSelector(role));
    var out = [];
    for(var i = 0; i < all.length; i++) {
        if(name == null || accessibleName(all[i], null) === name) out.push(all[i]);
    }
    return new UC(out, 'role=' + role);
};

window.expect = function(actual) {
    if(actual && actual.nodeType && !(actual instanceof U)) { actual = el(actual); }
    function build(neg) {
        function check(cond, msg) {
            if(neg ? cond : !cond) throw new Error((neg ? 'not: ' : '') + msg);
        }
        var api = {
            toBe: function(e) { check(actual === e, 'expected ' + JSON.stringify(e) + ' got ' + JSON.stringify(actual)); },
            toEqual: function(e) { check(actual === e, 'expected ' + JSON.stringify(e) + ' got ' + JSON.stringify(actual)); },
            toContain: function(e) { check(('' + actual).indexOf(e) >= 0, 'expected ' + JSON.stringify(actual) + ' to contain ' + JSON.stringify(e)); },
            toBeTruthy: function() { check(!!actual, 'expected truthy, got ' + JSON.stringify(actual)); },
            toBeFalsy: function() { check(!actual, 'expected falsy, got ' + JSON.stringify(actual)); },
            toHaveText: function(e) { check(actual && actual.text() === e, 'expected text ' + JSON.stringify(e) + ', got ' + JSON.stringify(actual ? actual.text() : null)); },
            toContainText: function(e) { check(actual && ((actual.text() || '').indexOf(e) >= 0), 'expected text containing ' + JSON.stringify(e) + ', got ' + JSON.stringify(actual ? actual.text() : null)); },
            toHaveAttribute: function(n, v) { var got = actual ? actual.attr(n) : null; check(v === undefined ? got != null : got === v, 'expected attribute ' + n + '=' + v + ', got ' + JSON.stringify(got)); },
            toHaveCount: function(e) { check(actual && actual.count() === e, 'expected count ' + e + ', got ' + (actual ? actual.count() : 0)); },
            toHaveClass: function(c) { check(actual && actual.hasClass && actual.hasClass(c), 'expected class ' + c); },
            toBeVisible: function() { checkVisible(neg, actual); },
            toBeHidden: function() { checkHidden(neg, actual); },
            toHaveValue: function(e) { check(actual && actual.value() === e, 'expected value ' + JSON.stringify(e) + ', got ' + JSON.stringify(actual ? actual.value() : null)); },
            toBeDisabled: function() { check(actual && actual.isDisabled(), 'expected element to be disabled'); },
            toBeEnabled: function() { check(actual && !actual.isDisabled(), 'expected element to be enabled'); },
            toBeChecked: function() { var ok = !!(actual && actual.isChecked()); if(neg ? ok : !ok) throw new Error((neg ? 'not: ' : '') + 'expected element to be checked [' + (actual && actual.el ? 'tag=' + actual.el.tagName + ' name=' + actual.el.getAttribute('name') + ' checked=' + actual.el.checked + ' defaultChecked=' + actual.el.defaultChecked : 'element not found') + ']'); },
            toBeFocused: function() { check(actual && actual.isFocused(), 'expected element to be focused'); },
            toHaveCSS: function(prop, v) { check(actual && actual.css(prop) === v, 'expected css ' + prop + '=' + v + ', got ' + (actual ? actual.css(prop) : null)); },
            toHaveJSProperty: function(n, v) { check(actual && actual.jsProp(n) === v, 'expected property ' + n + '=' + v); },
            toBeEmpty: function() { check(actual && ((actual.text() || '').length === 0), 'expected element to be empty'); },
            toHaveLength: function(n) { check(actual && actual.length === n, 'expected length ' + n); },
            toBeGreaterThan: function(e) { check(actual > e, 'expected ' + actual + ' > ' + e); },
            toBeGreaterThanOrEqual: function(e) { check(actual >= e, 'expected ' + actual + ' >= ' + e); },
            toBeLessThan: function(e) { check(actual < e, 'expected ' + actual + ' < ' + e); },
            toBeLessThanOrEqual: function(e) { check(actual <= e, 'expected ' + actual + ' <= ' + e); }
        };
        Object.defineProperty(api, 'not', { get: function() { return build(!neg); } });
        return api;
    }
    return build(false);
};
window.sleep = function(ms) { return new Promise(function(r) { setTimeout(r, ms); }); };
window.t = {
    // Everything logged since page load, including before the first test ran.
    // This is the only way to assert on something that happens during
    // initialisation -- hydration above all, which completes before any test body
    // is invoked.
    consoleAll: function() { return window.__ut_console_all.slice(); },
    hydrationMismatches: function() {
        var out = [];
        var all = window.__ut_console_all || [];
        for (var i = 0; i < all.length; i++) { if (all[i].indexOf('hydration mismatch') >= 0) out.push(all[i]); }
        return out;
    },
    sleep: window.sleep,
    waitFor: window.sleep,
    log: function(m) { console.log('[test] ' + m); },
    skip: function(m) { throw new Error('SKIP: ' + m); },
    consoleErrors: function() { return window.__ut_errors.slice(); },
    eval: function(js) { return window.eval(js); },
    // Global keyboard: dispatches on the focused element (Playwright page.keyboard).
    // Synthetic keydown has no default action, so Tab is emulated by moving focus
    // to the next/previous focusable element (matching browser Tab navigation).
    press: function(k) {
        var target = document.activeElement || document.body;
        var down = new KeyboardEvent('keydown', { key: k, bubbles: true, cancelable: true });
        target.dispatchEvent(down);
        if(k === 'Tab' && !down.defaultPrevented) {
            var sel = 'a[href],button:not([disabled]),input:not([disabled]),select:not([disabled]),textarea:not([disabled]),[tabindex]:not([tabindex="-1"])';
            var all = document.querySelectorAll(sel);
            var list = [];
            for(var i = 0; i < all.length; i++) {
                if(all[i].isConnected && all[i].offsetParent !== null) list.push(all[i]);
            }
            if(list.length === 0) {
                for(var j = 0; j < all.length; j++) { if(all[j].isConnected) list.push(all[j]); }
            }
            if(list.length > 0) {
                var idx = list.indexOf(document.activeElement);
                var next = list[(idx + 1) % list.length];
                if(next) next.focus();
            }
        }
        target.dispatchEvent(new KeyboardEvent('keyup', { key: k, bubbles: true }));
    },
    type: function(s) {
        var target = document.activeElement;
        if(!target) return;
        target.value = (target.value || '') + s;
        target.dispatchEvent(new Event('input', { bubbles: true }));
        target.dispatchEvent(new Event('change', { bubbles: true }));
    }
};

function setScope(name) {
    window.__ut_scope = null;
    var all = document.querySelectorAll('[data-ut]');
    for(var i = 0; i < all.length; i++) {
        if(all[i].getAttribute('data-ut') === name) { window.__ut_scope = all[i]; return; }
    }
}
// Report one result and continue. Never throws and never stalls the chain: the
// bridge is the only thing that ends the host's message loop, so a failure here
// would hang the whole suite.
function report(name, ok, msg, next) {
    var done = function() { if(next) next(); };
    try {
        if(!window.__webview__ || typeof window.__webview__.call !== 'function') { done(); return; }
        window.__webview__.call('result', name, !!ok, '' + msg).then(done, done);
    } catch(e) {
        done();
    }
}
function finishAll() {
    try { if(window.__webview__) window.__webview__.call('all_done', true); } catch(e) {}
}
// The order to run in. The host's expected list wins over what registered:
// it is what the user asked for, and it survives a fixture failing to register.
function utExpected() {
    var exp = window.__ut_expected;
    if(exp && exp.length) return exp;
    return Object.keys(window.__ut_tests);
}
function utMissingMessage() {
    var errs = window.__ut_page_errors;
    if(errs && errs.length) {
        return 'the page script failed to run, so this test never registered: ' + errs.join('; ') +
               ' -- dump the bundle with UT_DUMP_JS=1 and check it with `node --check`';
    }
    return 'the page script never registered this test (its JS bundle probably fails to parse -- dump it with UT_DUMP_JS=1 and check it with `node --check`)';
}
var UT_TIMEOUT_MS = 15000;
function runNext() {
    var names = utExpected();
    var i = 0;
    function step() {
        if(i >= names.length) { finishAll(); return; }
        var name = names[i++];
        var fn = window.__ut_tests[name];
        // Its `__ut_register` never ran, i.e. the bundle that registers it did
        // not parse (or threw before reaching it). Fail it by name and move on:
        // staying silent here is what left the host blocked forever.
        if(typeof fn !== 'function') { report(name, false, utMissingMessage(), step); return; }
        setScope(name);
        window.__ut_errors = [];
        window.__ut_console = [];
        var finished = false;
        var timer = setTimeout(function() {
            if(finished) return;
            finished = true;
            report(name, false, 'timed out after ' + UT_TIMEOUT_MS + 'ms', step);
        }, UT_TIMEOUT_MS);
        function finish(ok, msg) {
            if(finished) return;
            finished = true;
            clearTimeout(timer);
            report(name, ok, msg, step);
        }
        function failed(e) {
            var m = '' + (e && e.message ? e.message : e);
            if(m.indexOf('SKIP:') === 0) { finish(true, m); return; }
            var extra = window.__ut_console;
            if(extra && extra.length) m += ' | console: ' + extra.join('; ');
            finish(false, m);
        }
        try {
            var r = fn(window.t);
            Promise.resolve(r).then(function() {
                var errs = window.__ut_errors;
                if(errs.length) finish(false, 'uncaught: ' + errs.join('; '));
                else finish(true, '');
            }, failed);
        } catch(e) {
            failed(e);
        }
    }
    step();
}
// The harness runs from the head, so `load` has not fired yet in the normal
// case. The already-loaded branch covers a host that injects the page into a
// live document, where waiting for `load` would never start the run.
function utKick() { setTimeout(runNext, 120); }
if(document.readyState === 'complete') { utKick(); } else { window.addEventListener('load', utKick); }
})();
"""
