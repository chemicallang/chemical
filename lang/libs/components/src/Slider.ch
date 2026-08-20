// Shadcn-style Slider: a horizontal track with a draggable thumb.
//
// Props:
//   value         controlled current value (number)
//   defaultValue  uncontrolled initial value (default 0)
//   min / max     range bounds (default 0 / 100)
//   step          increment granularity (default 1)
//   onValueChange receives the new value (number) on every change
//   disabled      locks interaction
//   ariaLabel     accessible name
//   className     merged with the generated style class
//
// Interaction: click (or tap) the track to jump, ArrowLeft/ArrowRight /
// ArrowUp/ArrowDown adjust by one step, Home/End jump to the bounds.
// Works in both controlled (`value` set) and uncontrolled modes.
//
// Note: thumb/range positions use CSS `calc()` with the value embedded as a
// literal so the SSR HTML matches the hydrated first render exactly.

func slider_styles(page : &mut HtmlPage) : *char {
    return #css {
        position: relative;
        display: flex;
        align-items: center;
        width: 100%;
        height: 1.25rem;
        touch-action: none;
        user-select: none;
        .chx-slider-track {
            position: relative;
            width: 100%;
            height: 6px;
            border-radius: 999px;
            background: hsl(var(--secondary));
            overflow: visible;
            flex-shrink: 1;
            cursor: pointer;
        }
        .chx-slider-range {
            position: absolute;
            left: 0;
            top: 0;
            height: 100%;
            border-radius: 999px;
            background: hsl(var(--primary));
            pointer-events: none;
        }
        .chx-slider-thumb {
            position: absolute;
            top: 50%;
            width: 1.25rem;
            height: 1.25rem;
            border-radius: 999px;
            background: hsl(var(--background));
            border: 2px solid hsl(var(--primary));
            box-shadow: var(--shadow-sm);
            transform: translate(-50%, -50%);
            transition: box-shadow 0.15s ease;
            cursor: grab;
            &:active {
                cursor: grabbing;
            }
            &:focus-visible {
                outline: 2px solid hsl(var(--ring));
                outline-offset: 2px;
                box-shadow: 0 0 0 6px hsl(var(--ring) / 0.18);
            }
        }
        &[data-disabled="true"] {
            opacity: 0.5;
            pointer-events: none;
        }
    }
}

public #universal Slider(props) {
    state value = props.defaultValue != null ? props.defaultValue : (props.min || 0)
    state dragging = false
    var min = props.min || 0
    var max = props.max || 100
    var step = props.step || 1
    var current = props.value != null ? props.value : value
    var range = max - min
    var disabled = props.disabled || false
    const trackRef = useRef(null)
    var setValue = (v) => {
        var clamped = v
        if(clamped < min) { clamped = min }
        if(clamped > max) { clamped = max }
        if(props.value != null) {
            if(props.onValueChange) {
                props.onValueChange(clamped)
            }
        } else {
            value = clamped
            if(props.onValueChange) {
                props.onValueChange(clamped)
            }
        }
    }
    var valueFromX = (clientX) => {
        if(!trackRef.current) { return current }
        var rect = trackRef.current.getBoundingClientRect()
        if(rect.width == 0) { return current }
        var ratio = (clientX - rect.left) / rect.width
        var raw = min + ratio * range
        var stepped = Math.round((raw - min) / step) * step + min
        return stepped
    }
    // Click/tap on the track: compute the value from the horizontal position.
    var handleTrackClick = (e) => {
        if(disabled) {
            return
        }
        setValue(valueFromX(e.clientX))
    }
    // Drag: mousedown on thumb starts, mousemove on document tracks, mouseup ends.
    var handleThumbMouseDown = (e) => {
        if(disabled) {
            return
        }
        e.preventDefault()
        dragging = true
        var onMove = (me) => {
            if(!dragging) { return }
            setValue(valueFromX(me.clientX))
        }
        var onUp = () => {
            dragging = false
            document.removeEventListener("mousemove", onMove)
            document.removeEventListener("mouseup", onUp)
        }
        document.addEventListener("mousemove", onMove)
        document.addEventListener("mouseup", onUp)
    }
    // Touch drag support
    var handleThumbTouchStart = (e) => {
        if(disabled) {
            return
        }
        dragging = true
        var onMove = (te) => {
            if(!dragging) { return }
            if(te.touches.length > 0) {
                setValue(valueFromX(te.touches[0].clientX))
            }
        }
        var onEnd = () => {
            dragging = false
            document.removeEventListener("touchmove", onMove)
            document.removeEventListener("touchend", onEnd)
        }
        document.addEventListener("touchmove", onMove)
        document.addEventListener("touchend", onEnd)
    }
    var handleKeyDown = (e) => {
        if(disabled) {
            return
        }
        if(e.key == "ArrowRight" || e.key == "ArrowUp") {
            e.preventDefault()
            setValue(current + step)
        } else if(e.key == "ArrowLeft" || e.key == "ArrowDown") {
            e.preventDefault()
            setValue(current - step)
        } else if(e.key == "Home") {
            e.preventDefault()
            setValue(min)
        } else if(e.key == "End") {
            e.preventDefault()
            setValue(max)
        }
    }
    var classes = props.class || ""
    if(props.className) { classes = props.className }
    // Position via CSS calc so no JS arithmetic is needed at SSR time:
    // left = (current - min) / (max - min) * 100%
    var posStyle = "left:calc((" + current + " - " + min + ") / (" + max + " - " + min + ") * 100%);"
    return <div class={classes + " " + ${slider_styles(page)}} data-disabled={disabled ? "true" : "false"} style={props.style}>
        <div class="chx-slider-track" ref={trackRef} onClick={handleTrackClick}>
            <div class="chx-slider-range" style={"width:" + "calc((" + current + " - " + min + ") / (" + max + " - " + min + ") * 100%);"}></div>
            <div class="chx-slider-thumb" role="slider" tabindex="0" aria-valuemin={min} aria-valuemax={max} aria-valuenow={current} aria-label={props.ariaLabel || "Slider"} onKeyDown={handleKeyDown} onMouseDown={handleThumbMouseDown} onTouchStart={handleThumbTouchStart} style={posStyle}></div>
        </div>
    </div>
}
