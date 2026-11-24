window.$__uni_hydration_queue = []
window.$__uni_batch_depth = 0
window.$__uni_pending_instances = []
window.$__uni_error = ((message, details = "", cause = null) => {
    const suffix = details ? ": " + details : "";
    const err = new Error(message + suffix);
    if(cause) err.cause = cause;
    throw err;
})
window.$__uni_dispatch = ((fnName, target, props, mode = "children") => {        if(!target) {
        // A dispatch can miss its target when the server did not render the
        // component's boundary (e.g. an SSR bug, or a client-only subtree).
        // Never throw here: dispatches are emitted as sequential statements, so
        // an uncaught error would abort every later component's hydration.
        console.error("universal dispatch: mount target is missing for " + fnName + " (component was not server-rendered; skipping)");
        return;
    }
    const fn = window[fnName]
    if(fn) {
        // Memo check: if component is memoized and props haven't changed, skip mount
        if(fn.__uni_memo && target.__uni_prev_props) {
            const eq = fn.__uni_are_equal(target.__uni_prev_props, props || {});
            if(eq) return;
        }
        if(target) target.__uni_prev_props = props ? { ...props } : {};
        try {
            window.$__uni_mount(target, fn, props, mode);
        } catch(err) {
            console.error("universal mount failed for component", fnName, err);
        }
    } else {
        window.$__uni_hydration_queue.push([ fnName, target, props, mode ]);
    }
})
// Compact dispatch emitted by the converters: the element lookup and the
// mode string live here, so a server-rendered page spends one number and one
// name per instance instead of the getElementById boilerplate it used to
// repeat 800+ times (41% of a page bundle).
window.$__uni_c = ((id, fnName, props, root) => {
    const target = root ? window.$__uni_boundary('u' + id) : document.getElementById('u' + id)
    window.$__uni_dispatch(fnName, target, props, root ? "root" : "children")
})
window.$__uni_flush_batch = (() => {        const pending = window.$__uni_pending_instances;
    for(let i = 0; i < pending.length; i++) {
        const inst = pending[i];
        inst._pendingEffects = false;
        if(inst.layoutEffects && inst.layoutEffects.length) window.$__uni_run_effects(inst, inst.layoutEffects);
        if(inst.effects && inst.effects.length) window.$__uni_run_effects(inst, inst.effects);
    }
    pending.length = 0;
})
window.$__uni_batch = ((fn) => {
    window.$__uni_batch_depth++;
    try {
        fn();
    } finally {
        window.$__uni_batch_depth--;
        if(window.$__uni_batch_depth === 0) window.$__uni_flush_batch();
    }
})
