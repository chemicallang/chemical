// The in-page JavaScript harness for #universal_test. It is appended to the
// page's JS bundle before the per-test __ut_register calls. Keep every byte
// ASCII (the page JS buffer is scanned with signed chars elsewhere).

public const UT_HARNESS : *char = """
(function(){
'use strict';
window.__ut_tests = {};
window.__ut_errors = [];
window.__ut_scope = null;
window.__ut_register = function(name, isolate, fn) { window.__ut_tests[name] = fn; };
window.addEventListener('error', function(e){ window.__ut_errors.push('' + (e && e.message ? e.message : e)); });
window.addEventListener('unhandledrejection', function(e){ window.__ut_errors.push('' + (e && e.reason ? e.reason : e)); });

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
    var all = this.el.querySelectorAll(roleSelector(role));
    if(name == null) return all.length ? el(all[0], 'role=' + role) : el(null, 'role=' + role);
    for(var i = 0; i < all.length; i++) { if(accessibleName(all[i]) === name) return el(all[i], 'role=' + role + ' name=' + name); }
    for(var j = 0; j < all.length; j++) { if(accessibleName(all[j]).indexOf(name) >= 0) return el(all[j], 'role=' + role + ' name=' + name); }
    return el(null, 'role=' + role + ' name=' + name);
};
U.prototype.getByRoleAll = function(role, opts) {
    if(!this.el) return new UC([], 'role=' + role);
    var name = opts && opts.name != null ? opts.name : null;
    var all = this.el.querySelectorAll(roleSelector(role));
    var out = [];
    for(var i = 0; i < all.length; i++) {
        if(name == null || accessibleName(all[i]) === name) out.push(all[i]);
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
function accessibleName(node) {
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
            var ref = document.getElementById(ids[i]);
            if(ref) parts.push(ref.textContent || '');
        }
        var joined = parts.join(' ').trim();
        if(joined) return joined;
    }
    return (node.textContent || '').trim();
}
window.byRole = function(role, opts) {
    var name = opts && opts.name != null ? opts.name : null;
    var all = allIn(roleSelector(role));
    if(name == null) return all.length ? el(all[0], 'role=' + role) : el(null, 'role=' + role);
    for(var i = 0; i < all.length; i++) { if(accessibleName(all[i]) === name) return el(all[i], 'role=' + role + ' name=' + name); }
    for(var j = 0; j < all.length; j++) { if(accessibleName(all[j]).indexOf(name) >= 0) return el(all[j], 'role=' + role + ' name=' + name); }
    return el(null, 'role=' + role + ' name=' + name);
};
window.byRoleAll = function(role, opts) {
    var name = opts && opts.name != null ? opts.name : null;
    var all = allIn(roleSelector(role));
    var out = [];
    for(var i = 0; i < all.length; i++) {
        if(name == null || accessibleName(all[i]) === name) out.push(all[i]);
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
            toBeVisible: function() { check(actual && actual.isVisible(), 'expected element to be visible'); },
            toBeHidden: function() { check(actual && !actual.isVisible(), 'expected element to be hidden'); },
            toHaveValue: function(e) { check(actual && actual.value() === e, 'expected value ' + JSON.stringify(e) + ', got ' + JSON.stringify(actual ? actual.value() : null)); },
            toBeDisabled: function() { check(actual && actual.isDisabled(), 'expected element to be disabled'); },
            toBeEnabled: function() { check(actual && !actual.isDisabled(), 'expected element to be enabled'); },
            toBeChecked: function() { check(actual && actual.isChecked(), 'expected element to be checked'); },
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
function report(name, ok, msg, next) {
    window.__webview__.call('result', name, !!ok, '' + msg).then(next, next);
}
var UT_TIMEOUT_MS = 15000;
function runNext() {
    var names = Object.keys(window.__ut_tests);
    var i = 0;
    function step() {
        if(i >= names.length) { window.__webview__.call('all_done', true); return; }
        var name = names[i++];
        setScope(name);
        window.__ut_errors = [];
        var fn = window.__ut_tests[name];
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
window.addEventListener('load', function() { setTimeout(runNext, 120); });
})();
"""
