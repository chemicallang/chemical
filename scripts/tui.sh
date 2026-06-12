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
            BoolWidget("libs", "--libs", "--libs"),
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

# ─── Custom Commands ────────────────────────────────────────────
COMMANDS_FILE = os.path.join(CONFIG_DIR, "commands.json")

def load_commands():
    if not os.path.exists(COMMANDS_FILE):
        return []
    try:
        with open(COMMANDS_FILE) as f:
            return json.load(f)
    except:
        return []

def save_commands(cmds):
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(COMMANDS_FILE, "w") as f:
        json.dump(cmds, f, indent=2)

# ─── Named Configs (single file) ────────────────────────────────
CONFIGS_FILE = os.path.join(CONFIG_DIR, "configs.json")

def load_all_configs():
    if not os.path.exists(CONFIGS_FILE):
        return {}
    try:
        with open(CONFIGS_FILE) as f:
            return json.load(f)
    except:
        return {}

def save_all_configs(configs):
    os.makedirs(CONFIG_DIR, exist_ok=True)
    with open(CONFIGS_FILE, "w") as f:
        json.dump(configs, f, indent=2)

def save_named_config(name, sections, run_order):
    configs = load_all_configs()
    configs[name] = {"sections": to_dict(sections), "run": run_order}
    save_all_configs(configs)

def load_named_config(name, sections):
    """Load widget state for a named config. Returns run list, or None if not found."""
    configs = load_all_configs()
    if name not in configs:
        return None
    from_dict(sections, configs[name]["sections"])
    return configs[name].get("run", [])

# ─── Handle --run (stdlib only) ─────────────────────────────────
if len(sys.argv) > 1 and sys.argv[1] == "--run":
    name = sys.argv[2] if len(sys.argv) > 2 else ""
    if not name:
        print("Usage: --run <name>", file=sys.stderr)
        sys.exit(1)

    # Try custom command first (case-insensitive name match)
    for cmd in load_commands():
        if cmd["name"].lower() == name.lower():
            if CMDS_FILE:
                with open(CMDS_FILE, "w") as f:
                    f.write("set -e\n")
                    f.write("cd " + shlex.quote(REPO_ROOT) + "\n")
                    f.write(cmd["command"] + "\n")
            sys.exit(0)

    # Fall back to named config in configs.json
    configs = load_all_configs()
    matched = None
    for k in configs:
        if k.lower() == name.lower():
            matched = k
            break
    if matched is None:
        print(f"Config '{name}' not found", file=sys.stderr)
        sys.exit(1)
    sections = build_sections()
    run_order = load_named_config(matched, sections)
    cmds = []
    for sec in sections:
        if sec.name not in run_order:
            continue
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
    return "".join(S[s] for s in styles if s) + text + S["rst"]

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
                    k = sys.stdin.read(1)
                    return {"A": "up", "B": "down", "C": "right", "D": "left", "Z": "s_tab"}.get(
                        k, ""
                    )
                return nxt if nxt else ch
            return ch
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old)

def read_line(prompt):
    """Read a line of text in raw mode (returns when Enter pressed)."""
    sys.stdout.write(prompt)
    sys.stdout.write("\033[?25h")
    sys.stdout.flush()
    buf = ""
    while True:
        ch = read_key()
        if ch in ("\r", "\n"):
            sys.stdout.write("\n")
            sys.stdout.flush()
            break
        elif ch in ("\x7f", "\b"):
            if buf:
                buf = buf[:-1]
                sys.stdout.write("\b \b")
                sys.stdout.flush()
        elif len(ch) == 1 and ord(ch) >= 32:
            buf += ch
            sys.stdout.write(ch)
            sys.stdout.flush()
    sys.stdout.write("\033[?25l")
    sys.stdout.flush()
    return buf.strip()

def run_tui(sections):
    custom_commands = load_commands()
    tab_names = ["Test", "Build", "Configure", "Setup", "Configs", "Commands"]
    current_tab = 0
    focus_idx = 0
    commands_to_run = []
    msg = ""
    msg_err = False
    editing = False
    edit_w = None
    edit_buf = ""

    def sclear():
        sys.stdout.write("\033[H\033[J")

    def save_config(name, run_order):
        save_named_config(name, sections, run_order)
        return f"saved '{name}' (run: {', '.join(run_order)})"

    def load_config(name):
        configs = load_all_configs()
        if name not in configs:
            return f"config '{name}' not found", []
        from_dict(sections, configs[name]["sections"])
        return f"loaded '{name}'", configs[name].get("run", [])

    def build_tab_items():
        items_list = []
        for si, sec in enumerate(sections):
            items = [("section_run", si)]
            for w in sec.widgets:
                items.append(("section_widget", si, w))
            items_list.append(items)
        configs = load_all_configs()
        items_list.append([("config", k) for k in sorted(configs.keys())])
        items_list.append([("cmd", ci) for ci in range(len(custom_commands))])
        return items_list

    tab_items = build_tab_items()

    def rebuild_configs_tab():
        nonlocal tab_items
        configs = load_all_configs()
        tab_items[4] = [("config", k) for k in sorted(configs.keys())]

    def rebuild_cmds_tab():
        nonlocal tab_items
        tab_items[5] = [("cmd", ci) for ci in range(len(custom_commands))]

    def current_items():
        return tab_items[current_tab]

    def render():
        out = ""
        out += c(" Chemical Build TUI  ", "bold", "white", "bg_blue") + "\n"

        # Tab bar
        for i, name in enumerate(tab_names):
            if i == current_tab:
                out += c(f" {name} ", "bold", "white", "bg_blue")
            else:
                out += c(f" {name} ", "white", "bg_black")
        out += "\n"
        out += c("─" * 60 + "\n", "dim")

        items = tab_items[current_tab]

        if current_tab < 4:
            sec = sections[current_tab]
            for fi, entry in enumerate(items):
                focused = (fi == focus_idx) and not editing
                etype = entry[0]
                if etype == "section_run":
                    if focused:
                        out += c(f"  [ Run {sec.name} ]\n", "bold", "black", "bg_green")
                    else:
                        out += c(f"  [ Run {sec.name} ]\n", "green")
                elif etype == "section_widget":
                    _, _, w = entry
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
        elif current_tab == 4:
            configs = load_all_configs()
            if not items:
                out += c("  No saved configs. Press s to save one.\n", "dim")
            else:
                for fi, entry in enumerate(items):
                    focused = (fi == focus_idx) and not editing
                    _, name = entry
                    run_list = ", ".join(configs.get(name, {}).get("run", []))
                    if focused:
                        out += c(f"   {name}\n", "rev")
                    else:
                        out += c(f"   {name}\n", "bold", "green")
                    out += c(f"       run: {run_list}\n", "dim")
        else:
            if not items:
                out += c("  No custom commands. Press n to add one.\n", "dim")
            else:
                for fi, entry in enumerate(items):
                    focused = (fi == focus_idx) and not editing
                    _, ci = entry
                    cmd_name = custom_commands[ci]["name"]
                    cmd_text = custom_commands[ci]["command"]
                    if focused:
                        out += c(f"   [{ci}] {cmd_name}\n", "rev")
                    else:
                        out += c(f"   [{ci}] {cmd_name}\n", "bold", "green")
                    out += c(f"       > {cmd_text}\n", "dim")

        if msg:
            out += c(f"\n  {msg}\n", "bold" if msg_err else "", "red" if msg_err else "yellow")

        out += c("─" + "\n", "dim")
        out += c("j/k:Nav Tab:Cycle Space:Tog Enter:Run r:All s:Save l:Load n:NewCmd d:DelCmd q:Quit\n", "white", "bg_black")

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
                items = tab_items[current_tab]
                if items:
                    focus_idx = (focus_idx + 1) % len(items)

            elif key in ("k", "up"):
                items = tab_items[current_tab]
                if items:
                    focus_idx = (focus_idx - 1) % len(items)

            elif key == "\t":
                current_tab = (current_tab + 1) % len(tab_names)
                focus_idx = 0

            elif key == "s_tab":
                current_tab = (current_tab - 1) % len(tab_names)
                focus_idx = 0

            elif key == " ":
                items = tab_items[current_tab]
                if not items: continue
                entry = items[focus_idx]
                if current_tab < 4:
                    if entry[0] == "section_widget":
                        _, _, w = entry
                        if isinstance(w, BoolWidget): w.toggle()
                        elif isinstance(w, ChoiceWidget): w.next()
                elif current_tab == 4:
                    if entry[0] == "config":
                        _, name = entry
                        msg, _ = load_config(name)
                        msg_err = False

            elif key in ("left",):
                items = tab_items[current_tab]
                if not items: continue
                entry = items[focus_idx]
                if entry[0] == "section_widget":
                    _, _, w = entry
                    if isinstance(w, ChoiceWidget): w.prev()

            elif key in ("right",):
                items = tab_items[current_tab]
                if not items: continue
                entry = items[focus_idx]
                if entry[0] == "section_widget":
                    _, _, w = entry
                    if isinstance(w, ChoiceWidget): w.next()

            elif key in ("\r", "\n"):
                items = tab_items[current_tab]
                if not items: continue
                entry = items[focus_idx]
                etype = entry[0]
                if etype == "section_run":
                    _, si = entry
                    sec = sections[si]
                    builder = COMMAND_BUILDERS.get(sec.name)
                    if builder:
                        commands_to_run = [builder(sec.widgets, {})]
                        break
                elif etype == "section_widget":
                    _, _, w = entry
                    if isinstance(w, TextWidget):
                        editing = True
                        edit_w = w
                        edit_buf = w.value
                    elif isinstance(w, BoolWidget): w.toggle()
                    elif isinstance(w, ChoiceWidget): w.next()
                elif etype == "cmd":
                    _, ci = entry
                    commands_to_run = [custom_commands[ci]["command"]]
                    break
                elif etype == "config":
                    _, name = entry
                    tmp_sections = build_sections()
                    run_order = load_named_config(name, tmp_sections)
                    if not run_order:
                        msg = f"config '{name}' has no run sections"
                        msg_err = True
                    else:
                        cmds = []
                        for sec in tmp_sections:
                            if sec.name not in run_order:
                                continue
                            builder = COMMAND_BUILDERS.get(sec.name)
                            if builder:
                                cmds.append(builder(sec.widgets, {}))
                        if cmds:
                            commands_to_run = cmds
                            break

            elif key == "r":
                commands_to_run = []
                for sec in sections:
                    builder = COMMAND_BUILDERS.get(sec.name)
                    if builder:
                        commands_to_run.append(builder(sec.widgets, {}))
                break

            elif key == "s":
                name = read_line("Save config as: ")
                if not name:
                    msg = "save cancelled"
                    msg_err = True
                else:
                    sec_names = [sec.name for sec in sections]
                    sel = {n: True for n in sec_names}
                    s_idx = 0
                    while True:
                        sclear()
                        out_lines = ["Select sections to run (j/k:Nav  Space:Tog  Enter:Done  Esc:Cancel):\n"]
                        for i, n in enumerate(sec_names):
                            chk = "X" if sel[n] else " "
                            line = f"  [{chk}] {n}"
                            if i == s_idx:
                                out_lines.append(c(line, "rev") + "\n")
                            else:
                                out_lines.append(line + "\n")
                        sys.stdout.write("".join(out_lines))
                        sys.stdout.flush()
                        k = read_key()
                        if k in ("j", "down"):
                            s_idx = (s_idx + 1) % len(sec_names)
                        elif k in ("k", "up"):
                            s_idx = (s_idx - 1) % len(sec_names)
                        elif k == " ":
                            sel[sec_names[s_idx]] = not sel[sec_names[s_idx]]
                        elif k in ("\r", "\n"):
                            run_order = [n for n in sec_names if sel[n]]
                            if run_order:
                                msg = save_config(name, run_order)
                                rebuild_configs_tab()
                                msg_err = False
                            else:
                                msg = "no sections selected, not saved"
                                msg_err = True
                            break
                        elif k == "\x1b":
                            msg = "save cancelled"
                            msg_err = True
                            break

            elif key == "l":
                all_configs = load_all_configs()
                names = sorted(all_configs.keys())
                if not names:
                    msg = "no saved configs"
                    msg_err = True
                else:
                    sclear()
                    lines = ["Available configs:"]
                    for i, nm in enumerate(names):
                        run_list = ", ".join(all_configs[nm].get("run", []))
                        lines.append(f"  [{i}] {nm}  ({run_list})")
                    lines.append("")
                    sys.stdout.write("\n".join(lines) + "\n")
                    sys.stdout.flush()
                    inp = read_line("Enter number or name: ")
                    if inp:
                        matched = None
                        try:
                            idx = int(inp)
                            if 0 <= idx < len(names):
                                matched = names[idx]
                            else:
                                msg = "invalid index"
                                msg_err = True
                        except ValueError:
                            for nm in names:
                                if nm.lower() == inp.lower():
                                    matched = nm
                                    break
                            if matched is None:
                                msg = f"config '{inp}' not found"
                                msg_err = True
                        if matched is not None:
                            msg, _ = load_config(matched)
                            msg_err = False

            elif key == "n":
                name = read_line("New command name: ")
                if name:
                    cmd_str = read_line("Command: ")
                    if cmd_str:
                        custom_commands.append({"name": name, "command": cmd_str})
                        save_commands(custom_commands)
                        rebuild_cmds_tab()
                        msg = f"added command '{name}'"
                        msg_err = False

            elif key == "d":
                items = tab_items[current_tab]
                if not items: continue
                entry = items[focus_idx]
                if entry[0] == "cmd":
                    _, ci = entry
                    cmd_name = custom_commands[ci]["name"]
                    confirm = read_line(f"Delete '{cmd_name}'? (y/n): ")
                    if confirm.lower() == "y":
                        custom_commands.pop(ci)
                        save_commands(custom_commands)
                        rebuild_cmds_tab()
                        msg = f"deleted command '{cmd_name}'"
                        msg_err = False
                        if focus_idx >= len(tab_items[current_tab]):
                            focus_idx = max(0, len(tab_items[current_tab]) - 1)
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
            has_section_cmds = any(isinstance(c, list) for c in commands)
            if not _msvc_on and has_section_cmds:
                f.write("export CHEMICAL_MSVC_AUTO=0\n")
            for cmd in commands:
                if isinstance(cmd, list):
                    f.write(" ".join(shlex.quote(c) for c in cmd) + "\n")
                else:
                    f.write(cmd + "\n")
ENDOFPYTHON

"$PYTHON" "$TMPFILE" "$@"

# If the TUI queued commands to run, execute them now in the normal terminal
if [ -s "$CMDS_FILE" ]; then
    echo ""
    echo "==> Executing queued commands..."
    bash "$CMDS_FILE"
fi
