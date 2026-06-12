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
        self.choices = choices
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
            ChoiceWidget("backend", "Backend", [("--tcc", "--tcc"), ("--llvm", "--llvm")], default=0),
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
            ChoiceWidget("target", "Target", [("--tcc", "--tcc"), ("--llvm", "--llvm"), ("--lsp", "--lsp"), ("--all", "--all")], default=0),
            TextWidget("jobs", "Jobs", "-j", default="", placeholder="auto"),
        ]),
        Section("Configure", "scripts/configure.sh", [
            BoolWidget("no_llvm", "--no-llvm", "--no-llvm"),
            BoolWidget("msvc_auto", "MSVC Auto Setup", "", default=True),
            ChoiceWidget("generator", "Generator", [
                ("Default (system)", ""),
                ("Ninja", "Ninja"),
                ("Unix Makefiles", "Unix Makefiles"),
                ("Visual Studio 17 2022", "Visual Studio 17 2022"),
                ("Visual Studio 16 2019", "Visual Studio 16 2019"),
                ("Xcode", "Xcode"),
                ("MinGW Makefiles", "MinGW Makefiles"),
                ("NMake Makefiles", "NMake Makefiles"),
                ("Custom...", "__custom__"),
            ]),
            TextWidget("custom_generator", "Custom Generator", "--generator", default="", placeholder="e.g. Ninja Multi-Config"),
        ]),
        Section("Setup", "scripts/setup.sh", [
            BoolWidget("with_llvm", "--with-llvm", "--with-llvm"),
        ]),
    ]

# ─── Command builders ────────────────────────────────────────────
def build_setup_cmd(widgets, opts):
    cmd = ["./scripts/setup.sh"]
    if by_key(widgets, "with_llvm").value: cmd.append("--with-llvm")
    return cmd

def build_configure_cmd(widgets, opts):
    cmd = ["./scripts/configure.sh"]
    if by_key(widgets, "no_llvm").value: cmd.append("--no-llvm")
    gen = by_key(widgets, "generator")
    gen_val = gen.flag
    if gen_val == "__custom__":
        cg = by_key(widgets, "custom_generator")
        if cg.value: cmd.extend(["--generator", cg.value])
    elif gen_val:
        cmd.extend(["--generator", gen_val])
    return cmd

def build_build_cmd(widgets, opts):
    cmd = ["./scripts/build.sh", by_key(widgets, "target").flag]
    wj = by_key(widgets, "jobs")
    if wj.value: cmd.extend(["-j", wj.value])
    return cmd

def build_test_cmd(widgets, opts):
    cmd = ["./scripts/test.sh", by_key(widgets, "backend").flag]
    for k in ["libs", "no_run", "no_build", "emit_c", "use_c", "g_flag", "cache", "cached_plugins"]:
        ww = by_key(widgets, k)
        if ww.value and ww.flag: cmd.append(ww.flag)
    cmd.extend(["--mode", by_key(widgets, "mode").flag])
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

# ─── Handle --run first (stdlib only) ───────────────────────────
if len(sys.argv) > 1 and sys.argv[1] == "--run":
    config_name = sys.argv[2] if len(sys.argv) > 2 else ""
    if not config_name:
        print("Usage: --run <config_name>", file=sys.stderr)
        sys.exit(1)
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
            cmds.append(builder(sec.widgets, {}))
    if cmds and CMDS_FILE:
        _msvc_on = True
        for sec in sections:
            if sec.name == "Configure":
                for w in sec.widgets:
                    if w.key == "msvc_auto" and not w.value:
                        _msvc_on = False
                break
        with open(CMDS_FILE, "w") as f:
            f.write("set -e\n")
            f.write("cd " + shlex.quote(REPO_ROOT) + "\n")
            if not _msvc_on:
                f.write("export CHEMICAL_MSVC_AUTO=0\n")
            for cmd in cmds:
                f.write(" ".join(shlex.quote(c) for c in cmd) + "\n")
    sys.exit(0)

# ─── Cross-platform stdlib TUI (no external packages needed) ────

S = {
    "rst": "\033[0m",
    "bold": "\033[1m",
    "dim": "\033[2m",
    "black": "\033[30m",
    "red": "\033[31m",
    "green": "\033[32m",
    "yellow": "\033[33m",
    "blue": "\033[34m",
    "cyan": "\033[36m",
    "white": "\033[37m",
    "bg_blue": "\033[44m",
    "bg_green": "\033[42m",
    "bg_white": "\033[47m",
    "bg_black": "\033[40m",
    "rev": "\033[7m",
}

def c(text, *styles):
    return "".join(S[s] for s in styles) + text + S["rst"]

def read_key():
    """Cross-platform raw key reader (stdlib only)."""
    try:
        import msvcrt
        b = msvcrt.getch()
        if b == b"\xe0":
            return {"H": "up", "P": "down", "M": "right", "K": "left"}.get(
                msvcrt.getch().decode(), ""
            )
        ch = b.decode("utf-8", errors="replace")
        return ch
    except ImportError:
        import tty, termios
        fd = sys.stdin.fileno()
        old = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
            if ch == "\x1b":
                nxt = sys.stdin.read(1)
                if nxt == "[":
                    return {"A": "up", "B": "down", "C": "right", "D": "left"}.get(
                        sys.stdin.read(1), ""
                    )
                return nxt if nxt else ch
            return ch
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)

def run_tui(sections):
    flat = []
    for si, sec in enumerate(sections):
        flat.append((si, None))
        for w in sec.widgets:
            flat.append((si, w))

    focus_idx = 0
    commands_to_run = []
    msg = ""
    msg_err = False
    editing = False
    edit_w = None
    edit_buf = ""

    def sclear():
        sys.stdout.write("\033[H\033[J")

    def save_config(name):
        os.makedirs(CONFIG_DIR, exist_ok=True)
        with open(os.path.join(CONFIG_DIR, name), "w") as f:
            json.dump(to_dict(sections), f, indent=2)
        return f"saved '{name}'"

    def load_config(name):
        path = os.path.join(CONFIG_DIR, name)
        if not os.path.exists(path):
            return f"config '{name}' not found"
        with open(path) as f:
            from_dict(sections, json.load(f))
        return f"loaded '{name}'"

    def render():
        out = ""
        out += c(" Chemical Build TUI  ", "bold", "white", "bg_blue") + "\n"
        out += c("─" * 60 + "\n", "dim")

        wsi = 0
        for si, sec in enumerate(sections):
            out += c(f"\n  {sec.name}\n", "bold", "cyan")

            focused_btn = (wsi == focus_idx) and not editing
            if focused_btn:
                out += c(f"  [ Run {sec.name} ]\n", "bold", "black", "bg_green")
            else:
                out += c(f"  [ Run {sec.name} ]\n", "green")
            wsi += 1

            for w in sec.widgets:
                focused = (wsi == focus_idx) and not editing

                if isinstance(w, BoolWidget):
                    chk = "X" if w.value else " "
                    line = f"   [{chk}] {w.label}"
                    if focused:
                        out += c(line + "\n", "rev")
                    elif w.value:
                        out += c(line + "\n", "bold", "green")
                    else:
                        out += line + "\n"

                elif isinstance(w, ChoiceWidget):
                    line = f"   {w.label}: "
                    parts = []
                    for ci, (ctext, _) in enumerate(w.choices):
                        if ci == w.selected:
                            parts.append(c(f"(*) {ctext}", "bold", "green"))
                        else:
                            parts.append(c(f"( ) {ctext}", "dim"))
                    parts_str = "  ".join(parts)
                    if focused:
                        out += c(line, "rev") + parts_str + "\n"
                    else:
                        out += line + parts_str + "\n"

                elif isinstance(w, TextWidget):
                    if editing and w is edit_w:
                        val = edit_buf if edit_buf else w.placeholder
                        out += c(f"   {w.label}: [{val}] <-- typing\n", "rev")
                    else:
                        val = w.value if w.value else w.placeholder
                        line = f"   {w.label}: [{val}]"
                        if focused:
                            out += c(line + "\n", "rev")
                        else:
                            out += line + "\n"

                wsi += 1

        if msg:
            out += c(f"\n  {msg}\n", "bold" if msg_err else "", "red" if msg_err else "yellow")

        out += c("─" + "\n", "dim")
        out += c("j/k:Nav  Space:Tog  Enter:Run  r:All  s:Save  l:Load  q:Quit\n", "white", "bg_black")

        return out

    sys.stdout.write("\033[?25l")
    sys.stdout.flush()
    try:
        while True:
            sclear()
            sys.stdout.write(render())
            sys.stdout.flush()

            if editing:
                key = read_key()
                if key in ("\r", "\n"):
                    editing = False
                    edit_w.value = edit_buf
                elif key == "\x1b":
                    editing = False
                elif key in ("\x7f", "\b"):
                    edit_buf = edit_buf[:-1]
                elif len(key) == 1 and ord(key) >= 32:
                    edit_buf += key
                continue

            key = read_key()

            if key == "q":
                break
            elif key in ("j", "down"):
                focus_idx = (focus_idx + 1) % len(flat)
            elif key in ("k", "up"):
                focus_idx = (focus_idx - 1) % len(flat)
            elif key == " ":
                si, w = flat[focus_idx]
                if isinstance(w, BoolWidget): w.toggle()
                elif isinstance(w, ChoiceWidget): w.next()
            elif key in ("h", "left"):
                si, w = flat[focus_idx]
                if isinstance(w, ChoiceWidget): w.prev()
            elif key in ("l", "right"):
                si, w = flat[focus_idx]
                if isinstance(w, ChoiceWidget): w.next()
            elif key in ("\r", "\n"):
                si, w = flat[focus_idx]
                if w is None:
                    sec = sections[si]
                    builder = COMMAND_BUILDERS.get(sec.name)
                    if builder:
                        commands_to_run = [builder(sec.widgets, {})]
                        break
                elif isinstance(w, TextWidget):
                    editing = True
                    edit_w = w
                    edit_buf = w.value
                elif isinstance(w, BoolWidget): w.toggle()
                elif isinstance(w, ChoiceWidget): w.next()
            elif key == "r":
                commands_to_run = []
                for sec in sections:
                    builder = COMMAND_BUILDERS.get(sec.name)
                    if builder:
                        commands_to_run.append(builder(sec.widgets, {}))
                break
            elif key == "s":
                name = f"config-{datetime.datetime.now():%Y%m%d-%H%M%S}.json"
                msg = save_config(name)
                msg_err = False
            elif key == "l":
                os.makedirs(CONFIG_DIR, exist_ok=True)
                configs = [f for f in os.listdir(CONFIG_DIR)
                           if f.endswith(".json") and f != "last.json"]
                if not configs:
                    msg = "no saved configs"
                    msg_err = True
                else:
                    # Show configs and let user pick
                    sclear()
                    lines = ["Available configs:"]
                    for i, cfg in enumerate(configs):
                        lines.append(f"  [{i}] {cfg}")
                    lines.append("")
                    sys.stdout.write("\n".join(lines) + "\nEnter number or name: ")
                    sys.stdout.flush()
                    sys.stdout.write("\033[?25h")
                    sys.stdout.flush()
                    try:
                        inp = sys.stdin.readline().strip()
                    except:
                        inp = ""
                    sys.stdout.write("\033[?25l")
                    sys.stdout.flush()
                    if inp:
                        try:
                            idx = int(inp)
                            if 0 <= idx < len(configs):
                                msg = load_config(configs[idx])
                                msg_err = False
                            else:
                                msg = "invalid index"
                                msg_err = True
                        except ValueError:
                            nm = inp if inp.endswith(".json") else inp + ".json"
                            msg = load_config(nm)
                            msg_err = "not found" in msg
    finally:
        sys.stdout.write("\033[?25h")
        sys.stdout.flush()

    return commands_to_run


if __name__ == "__main__":
    sections = build_sections()
    last_cfg = os.path.join(CONFIG_DIR, "last.json")
    if os.path.exists(last_cfg):
        try:
            with open(last_cfg) as f:
                from_dict(sections, json.load(f))
        except:
            pass

    commands = run_tui(sections)

    try:
        os.makedirs(CONFIG_DIR, exist_ok=True)
        with open(last_cfg, "w") as f:
            json.dump(to_dict(sections), f)
    except:
        pass

    if commands and CMDS_FILE:
        _msvc_on = True
        for sec in sections:
            if sec.name == "Configure":
                for w in sec.widgets:
                    if w.key == "msvc_auto" and not w.value:
                        _msvc_on = False
                break
        with open(CMDS_FILE, "w") as f:
            f.write("set -e\n")
            f.write("cd " + shlex.quote(REPO_ROOT) + "\n")
            if not _msvc_on:
                f.write("export CHEMICAL_MSVC_AUTO=0\n")
            for cmd in commands:
                f.write(" ".join(shlex.quote(c) for c in cmd) + "\n")
ENDOFPYTHON

"$PYTHON" "$TMPFILE" "$@"

# If the TUI queued commands to run, execute them now in the normal terminal
if [ -s "$CMDS_FILE" ]; then
    echo ""
    echo "==> Executing queued commands..."
    bash "$CMDS_FILE"
fi
