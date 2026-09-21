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
    if(window.__ut_scope) return window.__ut_scope.querySelector(sel);
    return document.querySelector(sel);
}
function allIn(sel) {
    if(window.__ut_scope) return window.__ut_scope.querySelectorAll(sel);
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
    var s = window.getComputedStyle(this.el);
    return s.display !== 'none' && s.visibility !== 'hidden' && s.opacity !== '0';
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

window.$ = function(sel) { return el(queryIn(sel), sel); };
window.byCss = function(sel) { return el(queryIn(sel), sel); };
window.byTestId = function(id) { return el(queryIn('[data-testid="' + id + '"]'), 'testid=' + id); };
window.byLabel = function(lbl) { return el(queryIn('[aria-label="' + lbl + '"]'), 'label=' + lbl); };
window.byText = function(t) {
    var all = allIn('*');
    for(var i = 0; i < all.length; i++) {
        if(all[i].children.length === 0 && all[i].textContent === t) return el(all[i], 'text=' + t);
    }
    return el(null, 'text=' + t);
};
window.byRole = function(role, opts) {
    var name = opts && opts.name ? opts.name : null;
    var all = allIn('[role="' + role + '"]');
    for(var i = 0; i < all.length; i++) {
        if(name == null) return el(all[i], 'role=' + role);
        var t = (all[i].getAttribute('aria-label') || all[i].textContent || '').trim();
        if(t === name) return el(all[i], 'role=' + role + ' name=' + name);
    }
    return el(null, 'role=' + role + ' name=' + name);
};

window.expect = function(actual) {
    return {
        toBe: function(e) { if(actual !== e) throw new Error('expected ' + JSON.stringify(e) + ' but got ' + JSON.stringify(actual)); },
        toEqual: function(e) { if(actual !== e) throw new Error('expected ' + JSON.stringify(e) + ' but got ' + JSON.stringify(actual)); },
        toContain: function(e) { if(('' + actual).indexOf(e) < 0) throw new Error('expected ' + JSON.stringify(actual) + ' to contain ' + JSON.stringify(e)); },
        toBeTruthy: function() { if(!actual) throw new Error('expected truthy, got ' + JSON.stringify(actual)); },
        toHaveText: function(e) { if(!actual || actual.text() !== e) throw new Error('expected text ' + JSON.stringify(e) + ', got ' + JSON.stringify(actual ? actual.text() : null)); },
        toContainText: function(e) { var t = actual ? actual.text() : ''; if(!t || t.indexOf(e) < 0) throw new Error('expected text containing ' + JSON.stringify(e) + ', got ' + JSON.stringify(t)); },
        toHaveAttribute: function(n, v) { if(!actual || actual.attr(n) !== v) throw new Error('expected attribute ' + n + '=' + v); },
        toHaveCount: function(e) { if(!actual || actual.count() !== e) throw new Error('expected count ' + e + ', got ' + (actual ? actual.count() : 0)); },
        toHaveClass: function(c) { if(!actual || !actual.el || !actual.el.classList || !actual.el.classList.contains(c)) throw new Error('expected class ' + c); },
        toBeVisible: function() { if(!actual || !actual.isVisible()) throw new Error('expected element to be visible'); },
        toBeHidden: function() { if(actual && actual.el && actual.isVisible()) throw new Error('expected element to be hidden'); }
    };
};
window.sleep = function(ms) { return new Promise(function(r) { setTimeout(r, ms); }); };
window.t = {
    sleep: window.sleep,
    log: function(m) { console.log('[test] ' + m); },
    skip: function(m) { throw new Error('SKIP: ' + m); }
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
