#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Find Python 3
PYTHON=""
for cmd in python3 python; do
  if command -v "$cmd" &>/dev/null; then
    if "$cmd" -c "import sys; sys.exit(0 if sys.version_info >= (3,6) else 1)" 2>/dev/null; then
      PYTHON="$cmd"
      break
    fi
  fi
done

if [ -z "$PYTHON" ]; then
  echo "Error: Python 3.6+ is required but not found"
  exit 1
fi

export CHEMICAL_SCRIPT_DIR="$SCRIPT_DIR"
export CHEMICAL_REPO_ROOT="$REPO_ROOT"

# Temp files for Python script and queued commands
TMPFILE=$(mktemp /tmp/chemical-tui-XXXXXX.py)
CMDS_FILE=$(mktemp /tmp/chemical-tui-cmds-XXXXXX.sh)
trap 'rm -f "$TMPFILE" "$CMDS_FILE"' EXIT

export CHEMICAL_CMDS_FILE="$CMDS_FILE"

cat > "$TMPFILE" << 'ENDOFPYTHON'
import curses
import datetime
import json
import os
import shlex
import subprocess
import sys

SCRIPT_DIR = os.environ["CHEMICAL_SCRIPT_DIR"]
REPO_ROOT = os.environ["CHEMICAL_REPO_ROOT"]
CONFIG_DIR = os.path.join(SCRIPT_DIR, "tui-configs")
CMDS_FILE = os.environ.get("CHEMICAL_CMDS_FILE", "")

# ─── Color pairs ────────────────────────────────────────────────
C_HEADER     = 1
C_SECTION    = 2
C_FOCUS      = 3
C_CHECKED    = 4
C_CMD        = 5
C_FOOTER     = 6
C_ERROR      = 7
C_SUCCESS    = 8
C_RUN_BTN    = 9

# ─── Data model ──────────────────────────────────────────────────
class Widget:
    pass

class BoolWidget(Widget):
    def __init__(self, key, label, flag, default=False):
        self.key = key
        self.label = label
        self.flag = flag
        self.value = default
    def toggle(self): self.value = not self.value

class ChoiceWidget(Widget):
    def __init__(self, key, label, choices, default=0):
        self.key = key
        self.label = label
        self.choices = choices  # list of (display_text, flag_or_none)
        self.selected = default
    def prev(self): self.selected = (self.selected - 1) % len(self.choices)
    def next(self): self.selected = (self.selected + 1) % len(self.choices)
    @property
    def flag(self):
        _, f = self.choices[self.selected]
        return f

class TextWidget(Widget):
    def __init__(self, key, label, flag, default="", placeholder=""):
        self.key = key
        self.label = label
        self.flag = flag
        self.value = default
        self.placeholder = placeholder

class Section:
    def __init__(self, name, script_rel, widgets, can_build=True):
        self.name = name
        self.script_rel = script_rel
        self.script_abs = os.path.join(REPO_ROOT, script_rel)
        self.widgets = widgets
        self.can_build = can_build

MODES = ["debug_quick", "debug_complete", "debug", "release_safe", "release_fast"]

def build_sections():
    return [
        Section("Test", "scripts/test.sh", [
            ChoiceWidget("backend", "Backend", [
                ("--tcc", "--tcc"),
                ("--llvm", "--llvm"),
            ], default=0),
            BoolWidget("libs", "--libs", "--arg-test-libs"),
            BoolWidget("no_run", "--no-run", "--no-run"),
            BoolWidget("no_build", "--no-build", "--no-build"),
            BoolWidget("emit_c", "--emit-c", "--emit-c"),
            BoolWidget("use_c", "--use-c", "--use-c"),
            BoolWidget("g_flag", "-g", "-g"),
            BoolWidget("cache", "--cache", "--cache"),
            BoolWidget("cached_plugins", "--cached-plugins", "--cached-plugins"),
            ChoiceWidget("mode", "Mode", [(m, m) for m in MODES], default=0),
            TextWidget("output", "Output (-o)", "-o", default="", placeholder="default"),
            TextWidget("jobs_test", "Jobs", "-j", default="", placeholder="auto"),
        ]),
        Section("Build", "scripts/build.sh", [
            ChoiceWidget("target", "Target", [
                ("--tcc", "--tcc"),
                ("--llvm", "--llvm"),
                ("--lsp", "--lsp"),
                ("--all", "--all"),
            ], default=0),
            TextWidget("jobs", "Jobs", "-j", default="", placeholder="auto"),
        ]),
        Section("Configure", "scripts/configure.sh", [
            BoolWidget("no_llvm", "--no-llvm", "--no-llvm"),
        ]),
        Section("Setup", "scripts/setup.sh", [
            BoolWidget("with_llvm", "--with-llvm", "--with-llvm"),
        ]),
    ]

# ─── Command builders ────────────────────────────────────────────
def build_setup_cmd(widgets, opts):
    cmd = ["./scripts/setup.sh"]
    w = by_key(widgets, "with_llvm")
    if w.value: cmd.append("--with-llvm")
    return cmd

def build_configure_cmd(widgets, opts):
    cmd = ["./scripts/configure.sh"]
    w = by_key(widgets, "no_llvm")
    if w.value: cmd.append("--no-llvm")
    return cmd

def build_build_cmd(widgets, opts):
    cmd = ["./scripts/build.sh"]
    w = by_key(widgets, "target")
    cmd.append(w.flag)
    wj = by_key(widgets, "jobs")
    if wj.value: cmd.extend(["-j", wj.value])
    return cmd

def build_test_cmd(widgets, opts):
    cmd = ["./scripts/test.sh"]
    w = by_key(widgets, "backend")
    cmd.append(w.flag)
    for k in ["libs", "no_run", "no_build", "emit_c", "use_c", "g_flag", "cache", "cached_plugins"]:
        ww = by_key(widgets, k)
        if ww.value and ww.flag: cmd.append(ww.flag)
    wm = by_key(widgets, "mode")
    cmd.extend(["--mode", wm.flag])
    wo = by_key(widgets, "output")
    if wo.value: cmd.extend(["-o", wo.value])
    wj = by_key(widgets, "jobs_test")
    if wj.value: cmd.extend(["-j", wj.value])
    return cmd

COMMAND_BUILDERS = {
    "Setup": build_setup_cmd,
    "Configure": build_configure_cmd,
    "Build": build_build_cmd,
    "Test": build_test_cmd,
}

def by_key(widgets, key):
    for w in widgets:
        if w.key == key: return w
    return None

def to_dict(sections):
    d = {}
    for sec in sections:
        dd = {}
        for w in sec.widgets:
            if isinstance(w, BoolWidget): dd[w.key] = w.value
            elif isinstance(w, ChoiceWidget): dd[w.key] = w.selected
            elif isinstance(w, TextWidget): dd[w.key] = w.value
        d[sec.name] = dd
    return d

def from_dict(sections, d):
    for sec in sections:
        if sec.name not in d: continue
        dd = d[sec.name]
        for w in sec.widgets:
            if w.key in dd:
                if isinstance(w, BoolWidget): w.value = dd[w.key]
                elif isinstance(w, ChoiceWidget): w.selected = dd[w.key]
                elif isinstance(w, TextWidget): w.value = dd[w.key]

# ─── Safe rendering helpers ──────────────────────────────────────
def safe_addstr(win, y, x, text, attr=0):
    maxy, maxx = win.getmaxyx()
    if y >= maxy or x >= maxx: return
    avail = maxx - x - 1
    if avail <= 0: return
    # Simple char-based truncation
    text = text[:avail]
    try:
        win.addstr(y, x, text, attr)
    except curses.error:
        pass

# ─── TUI ─────────────────────────────────────────────────────────
def main(stdscr):
    curses.curs_set(0)
    curses.use_default_colors()
    curses.init_pair(C_HEADER,  curses.COLOR_WHITE,  curses.COLOR_BLUE)
    curses.init_pair(C_SECTION, curses.COLOR_CYAN,   -1)
    curses.init_pair(C_FOCUS,   curses.COLOR_BLACK,  curses.COLOR_WHITE)
    curses.init_pair(C_CHECKED, curses.COLOR_GREEN,  -1)
    curses.init_pair(C_CMD,     curses.COLOR_YELLOW,  -1)
    curses.init_pair(C_FOOTER,  curses.COLOR_WHITE,  curses.COLOR_BLACK)
    curses.init_pair(C_ERROR,   curses.COLOR_RED,    -1)
    curses.init_pair(C_SUCCESS, curses.COLOR_GREEN,   -1)
    curses.init_pair(C_RUN_BTN, curses.COLOR_BLACK,  curses.COLOR_GREEN)

    sections = build_sections()
    # Load last config if exists
    last_cfg = os.path.join(CONFIG_DIR, "last.json")
    if os.path.exists(last_cfg):
        try:
            with open(last_cfg) as f: from_dict(sections, json.load(f))
        except: pass

    # Build flat widget list for focus navigation
    flat = []
    for si, sec in enumerate(sections):
        flat.append((si, None))  # None = run button for this section
        for w in sec.widgets:
            flat.append((si, w))

    focus_idx = 0
    editing_text = False
    text_buf = ""
    msg = ""
    msg_pair = 0
    running = False
    commands_to_run = []

    def save_config(name):
        os.makedirs(CONFIG_DIR, exist_ok=True)
        with open(os.path.join(CONFIG_DIR, name), "w") as f:
            json.dump(to_dict(sections), f, indent=2)
        return f"saved '{name}'"

    def load_config(name):
        path = os.path.join(CONFIG_DIR, name)
        if not os.path.exists(path): return f"config '{name}' not found"
        with open(path) as f: from_dict(sections, json.load(f))
        nonlocal focus_idx; focus_idx = 0
        return f"loaded '{name}'"

    def make_cmds():
        cmds = []
        for sec in sections:
            builder = COMMAND_BUILDERS.get(sec.name)
            if builder: cmds.append((sec.name, builder(sec.widgets, {})))
        return cmds

    def queue_and_exit(cmds):
        nonlocal commands_to_run
        commands_to_run = cmds

    while True:
        try:
            stdscr.clear()
            maxy, maxx = stdscr.getmaxyx()
            y = 0

            if maxy < 15 or maxx < 50:
                safe_addstr(stdscr, 0, 0, "Terminal too small (need at least 50x15)")
                stdscr.refresh()
                stdscr.getch()
                continue

            # Header
            if y < maxy:
                stdscr.attron(curses.color_pair(C_HEADER))
                safe_addstr(stdscr, y, 0, " " * maxx)
                safe_addstr(stdscr, y, 1, "Chemical Build TUI")
                cfg_name = os.path.basename(last_cfg).replace(".json","") if os.path.exists(last_cfg) else "default"
                cfg_text = f"  [{cfg_name}]"
                safe_addstr(stdscr, y, 21, cfg_text, curses.A_DIM)
                if running:
                    status = " [running...]"
                    stdscr.attron(curses.color_pair(C_CMD))
                    safe_addstr(stdscr, y, maxx - len(status) - 1, status)
                    stdscr.attroff(curses.color_pair(C_CMD))
                stdscr.attroff(curses.color_pair(C_HEADER))
                y += 1

            if y < maxy:
                safe_addstr(stdscr, y, 0, "-" * (maxx - 1), curses.A_DIM)
                y += 1

            # Content
            wsi = 0
            for si, sec in enumerate(sections):
                if y >= maxy - 4: break
                stdscr.attron(curses.color_pair(C_SECTION) | curses.A_BOLD)
                safe_addstr(stdscr, y, 1, f"  {sec.name}")
                stdscr.attroff(curses.color_pair(C_SECTION) | curses.A_BOLD)
                y += 1

                # Run button first (above options)
                if y < maxy - 4:
                    focused_btn = (wsi == focus_idx)
                    btn_text = f"  [ Run {sec.name} ]  "
                    if focused_btn:
                        stdscr.attron(curses.color_pair(C_RUN_BTN))
                        safe_addstr(stdscr, y, 4, btn_text)
                        stdscr.attroff(curses.color_pair(C_RUN_BTN))
                    else:
                        safe_addstr(stdscr, y, 4, btn_text, curses.A_DIM)
                    y += 1
                wsi += 1

                for w in sec.widgets:
                    if y >= maxy - 4: break
                    prefix = " " * 3
                    focused = (wsi == focus_idx) and not editing_text
                    attr = curses.color_pair(C_FOCUS) if focused else curses.A_NORMAL

                    if isinstance(w, BoolWidget):
                        chk = "[X]" if w.value else "[ ]"
                        line = f"{prefix}{chk} {w.label}"
                        if w.value:
                            stdscr.attron(curses.color_pair(C_CHECKED))
                            safe_addstr(stdscr, y, 0, line)
                            stdscr.attroff(curses.color_pair(C_CHECKED))
                        else:
                            safe_addstr(stdscr, y, 0, line, attr)
                        y += 1

                    elif isinstance(w, ChoiceWidget):
                        line = f"{prefix}{w.label}: "
                        parts = []
                        for ci, (ctext, _) in enumerate(w.choices):
                            marker = "(*)" if ci == w.selected else "( )"
                            parts.append(f"{marker}{ctext}")
                        line += "  ".join(parts)
                        safe_addstr(stdscr, y, 0, line, attr if focused else 0)
                        y += 1

                    elif isinstance(w, TextWidget):
                        if editing_text and w is flat[focus_idx][1]:
                            val = text_buf if text_buf else w.placeholder
                            line = f"{prefix}{w.label}: [{val}]"
                            safe_addstr(stdscr, y, 0, line, curses.color_pair(C_FOCUS))
                            safe_addstr(stdscr, y, len(line), " <-- typing")
                        else:
                            val = w.value if w.value else w.placeholder
                            line = f"{prefix}{w.label}: [{val}]"
                            safe_addstr(stdscr, y, 0, line, attr if focused else 0)
                        y += 1

                    wsi += 1

            if y < maxy - 2:
                y += 1
                cmds = make_cmds()
                stdscr.attron(curses.color_pair(C_CMD) | curses.A_BOLD)
                safe_addstr(stdscr, y, 1, "Commands:")
                stdscr.attroff(curses.color_pair(C_CMD) | curses.A_BOLD)
                y += 1
                for cname, ccmd in cmds:
                    if y >= maxy - 1: break
                    line = "  " + " ".join(ccmd)
                    safe_addstr(stdscr, y, 2, line, curses.A_DIM)
                    y += 1

            if msg and maxy > 2:
                stdscr.attron(curses.color_pair(msg_pair) if msg_pair else curses.A_NORMAL)
                safe_addstr(stdscr, maxy - 2, 1, msg)
                stdscr.attroff(curses.color_pair(msg_pair) if msg_pair else curses.A_NORMAL)

            shortcuts = (
                "[Down/Up:Navigate] [Space:Toggle] [Enter:Run] "
                "[r:Run All] [s:Save] [l:Load] [q:Quit]"
            )
            if maxy > 1:
                stdscr.attron(curses.color_pair(C_FOOTER))
                safe_addstr(stdscr, maxy - 1, 0, " " * maxx)
                safe_addstr(stdscr, maxy - 1, 1, shortcuts)
                stdscr.attroff(curses.color_pair(C_FOOTER))

            stdscr.refresh()
        except curses.error:
            pass  # skip rendering errors

        # ─── Input handling ──────────────────────────────────────
        key = stdscr.getch()

        if editing_text:
            si, w = flat[focus_idx]
            if key == 27:  # Escape
                editing_text = False
                w.value = text_buf
            elif key in (10, 13, 9):  # Enter or Tab
                editing_text = False
                w.value = text_buf
                focus_idx = (focus_idx + 1) % len(flat)
            elif key == 263 or key == 127:  # Backspace
                text_buf = text_buf[:-1]
            elif key == 330:  # Delete
                pass
            elif 32 <= key <= 126:
                text_buf += chr(key)
            continue

        if key == ord("q"):
            # Save last config
            try:
                os.makedirs(CONFIG_DIR, exist_ok=True)
                with open(last_cfg, "w") as f: json.dump(to_dict(sections), f)
            except: pass
            break

        elif key == ord(" "):  # Space
            if focus_idx < len(flat):
                si, w = flat[focus_idx]
                if isinstance(w, BoolWidget):
                    w.toggle()
                elif isinstance(w, ChoiceWidget):
                    w.next()

        elif key in (curses.KEY_DOWN, 9):  # Down or Tab
            focus_idx = (focus_idx + 1) % len(flat)

        elif key in (curses.KEY_UP, 353, curses.KEY_BTAB if hasattr(curses, 'KEY_BTAB') else -1):  # Up or Shift+Tab
            focus_idx = (focus_idx - 1) % len(flat)

        elif key == curses.KEY_LEFT:
            if focus_idx < len(flat):
                si, w = flat[focus_idx]
                if isinstance(w, ChoiceWidget): w.prev()

        elif key == curses.KEY_RIGHT:
            if focus_idx < len(flat):
                si, w = flat[focus_idx]
                if isinstance(w, ChoiceWidget): w.next()

        elif key in (10, 13):  # Enter
            if focus_idx < len(flat):
                si, w = flat[focus_idx]
                if w is None:
                    sec = sections[si]
                    builder = COMMAND_BUILDERS.get(sec.name)
                    if builder:
                        cmd = builder(sec.widgets, {})
                        queue_and_exit([cmd])
                        break
                elif isinstance(w, TextWidget):
                    editing_text = True
                    text_buf = w.value
                elif isinstance(w, BoolWidget):
                    w.toggle()
                elif isinstance(w, ChoiceWidget):
                    w.next()

        elif key == ord("r"):
            cmds = []
            for sec in sections:
                builder = COMMAND_BUILDERS.get(sec.name)
                if builder:
                    cmd = builder(sec.widgets, {})
                    cmds.append(cmd)
            queue_and_exit(cmds)
            break

        elif key == ord("s"):
            os.makedirs(CONFIG_DIR, exist_ok=True)
            name = f"config-{datetime.datetime.now():%Y%m%d-%H%M%S}.json"
            msg = save_config(name)

        elif key == ord("l"):
            # Load config - show list
            os.makedirs(CONFIG_DIR, exist_ok=True)
            configs = [f for f in os.listdir(CONFIG_DIR) if f.endswith(".json") and f != "last.json"]
            if not configs:
                msg = "no saved configs"
                msg_pair = C_ERROR
            else:
                # Select from list using simple menu
                selected = menu_selector(stdscr, "Select config", configs)
                if selected:
                    msg = load_config(selected)

    # Write queued commands to the cmds file for the bash wrapper to execute
    if commands_to_run and CMDS_FILE:
        with open(CMDS_FILE, "w") as f:
            f.write("set -e\n")
            f.write("cd " + shlex.quote(REPO_ROOT) + "\n")
            for cmd in commands_to_run:
                f.write(" ".join(shlex.quote(c) for c in cmd) + "\n")

def menu_selector(stdscr, title, items):
    """Simple menu selector overlay."""
    maxy, maxx = stdscr.getmaxyx()
    height = min(len(items) + 4, maxy - 4)
    width = min(max(len(title) + 4, 40), maxx - 4)
    sy = (maxy - height) // 2
    sx = (maxx - width) // 2

    selected = 0
    while True:
        # Draw overlay
        stdscr.attron(curses.color_pair(C_HEADER))
        for dy in range(height):
            stdscr.addstr(sy + dy, sx, " " * width)
        stdscr.attroff(curses.color_pair(C_HEADER))
        safe_addstr(stdscr, sy, sx, f" {title} ".center(width))
        safe_addstr(stdscr, sy + 1, sx, "-" * width)

        for i, item in enumerate(items):
            display = item[:width-4]
            if i == selected:
                stdscr.attron(curses.color_pair(C_FOCUS))
                safe_addstr(stdscr, sy + 2 + i, sx + 2, f"> {display}")
                stdscr.attroff(curses.color_pair(C_FOCUS))
            else:
                safe_addstr(stdscr, sy + 2 + i, sx + 2, f"  {display}")

        safe_addstr(stdscr, sy + height - 1, sx, "-" * width)
        safe_addstr(stdscr, sy + height - 1, sx + 1, "Up/Down:move  Enter:select  q:cancel")
        stdscr.refresh()

        key = stdscr.getch()
        if key == ord("q") or key == 27: return None
        elif key == 10 or key == 13: return items[selected]
        elif key == curses.KEY_UP: selected = (selected - 1) % len(items)
        elif key == curses.KEY_DOWN: selected = (selected + 1) % len(items)
        elif key == 9: selected = (selected + 1) % len(items)

if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "--run" and len(args) >= 2:
        config_name = args[1]
        if not config_name.endswith(".json"):
            config_name += ".json"
        config_path = os.path.join(CONFIG_DIR, config_name)
        if not os.path.exists(config_path):
            print(f"Config '{config_name}' not found in {CONFIG_DIR}", file=sys.stderr)
            sys.exit(1)
        sections = build_sections()
        with open(config_path) as f:
            from_dict(sections, json.load(f))
        cmds = []
        for sec in sections:
            builder = COMMAND_BUILDERS.get(sec.name)
            if builder:
                cmd = builder(sec.widgets, {})
                cmds.append(cmd)
        if cmds and CMDS_FILE:
            with open(CMDS_FILE, "w") as f:
                f.write("set -e\n")
                f.write("cd " + shlex.quote(REPO_ROOT) + "\n")
                for cmd in cmds:
                    f.write(" ".join(shlex.quote(c) for c in cmd) + "\n")
        sys.exit(0)

    try:
        curses.wrapper(main)
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
ENDOFPYTHON

"$PYTHON" "$TMPFILE" "$@"

# If the TUI queued commands to run, execute them now in the normal terminal
if [ -s "$CMDS_FILE" ]; then
    echo ""
    echo "==> Executing queued commands..."
    bash "$CMDS_FILE"
fi
