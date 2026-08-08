#!/usr/bin/env bash

set -u

PROGRAM="TCCCompiler"
TIMESTAMP="$(date '+%Y-%m-%d_%H-%M-%S')"
OUTDIR="TCCCompiler_deadlock_${TIMESTAMP}"

mkdir -p "$OUTDIR"

echo "============================================================"
echo " TCCCompiler Deadlock Diagnostic Collector"
echo "============================================================"
echo
echo "Output directory:"
echo "  $OUTDIR"
echo

# ------------------------------------------------------------
# Find PID
# ------------------------------------------------------------

PIDS="$(pgrep -x "$PROGRAM" || true)"

if [ -z "$PIDS" ]; then
    echo "ERROR: Could not find a running process named '$PROGRAM'."
    exit 1
fi

PID_COUNT="$(echo "$PIDS" | wc -w)"

if [ "$PID_COUNT" -gt 1 ]; then
    echo "Multiple $PROGRAM processes found:"
    echo
    ps -fp $PIDS
    echo
    read -rp "Enter the PID you want to inspect: " PID
else
    PID="$PIDS"
fi

if ! kill -0 "$PID" 2>/dev/null; then
    echo "ERROR: PID $PID is no longer running."
    exit 1
fi

echo "PID: $PID"
echo

# ------------------------------------------------------------
# Basic process information
# ------------------------------------------------------------

echo "[1/12] Collecting process information..."

{
    echo "============================================================"
    echo "PROCESS INFORMATION"
    echo "============================================================"
    echo

    echo "PID:"
    echo "$PID"
    echo

    echo "Executable:"
    readlink -f "/proc/$PID/exe" 2>/dev/null || true
    echo

    echo "Command line:"
    tr '\0' ' ' < "/proc/$PID/cmdline" 2>/dev/null || true
    echo
    echo

    echo "Process status:"
    cat "/proc/$PID/status" 2>/dev/null || true
} > "$OUTDIR/process.txt"


# ------------------------------------------------------------
# Process limits
# ------------------------------------------------------------

echo "[2/12] Collecting process limits..."

cat "/proc/$PID/limits" > "$OUTDIR/limits.txt" 2>/dev/null || true


# ------------------------------------------------------------
# Memory mappings
# ------------------------------------------------------------

echo "[3/12] Collecting memory mappings..."

cat "/proc/$PID/maps" > "$OUTDIR/memory_maps.txt" 2>/dev/null || true

if [ -r "/proc/$PID/smaps" ]; then
    cat "/proc/$PID/smaps" > "$OUTDIR/memory_smaps.txt" 2>/dev/null || true
fi


# ------------------------------------------------------------
# File descriptors
# ------------------------------------------------------------

echo "[4/12] Collecting file descriptors..."

{
    echo "============================================================"
    echo "OPEN FILE DESCRIPTORS"
    echo "============================================================"
    echo

    for FD in /proc/"$PID"/fd/*; do
        if [ -e "$FD" ]; then
            printf "%s -> %s\n" "$FD" "$(readlink "$FD" 2>/dev/null || true)"
        fi
    done
} > "$OUTDIR/file_descriptors.txt"


# ------------------------------------------------------------
# Per-thread /proc information
# ------------------------------------------------------------

echo "[5/12] Collecting per-thread kernel information..."

THREAD_DIR="$OUTDIR/threads"
mkdir -p "$THREAD_DIR"

for TASK in /proc/"$PID"/task/*; do

    TID="$(basename "$TASK")"

    {
        echo "============================================================"
        echo "THREAD $TID"
        echo "============================================================"
        echo

        echo "----- STATUS -----"
        cat "$TASK/status" 2>/dev/null || true

        echo
        echo "----- SCHED -----"
        cat "$TASK/sched" 2>/dev/null || true

        echo
        echo "----- WCHAN -----"
        cat "$TASK/wchan" 2>/dev/null || true

        echo
        echo "----- KERNEL STACK -----"
        cat "$TASK/stack" 2>/dev/null || true

        echo
        echo "----- SYSCALL -----"
        cat "$TASK/syscall" 2>/dev/null || true

    } > "$THREAD_DIR/thread_${TID}.txt"

done


# ------------------------------------------------------------
# GDB command file
# ------------------------------------------------------------

echo "[6/12] Preparing GDB diagnostic commands..."

GDB_SCRIPT="$OUTDIR/gdb_commands.txt"

cat > "$GDB_SCRIPT" <<'EOF'
set pagination off
set confirm off
set print pretty on
set print object on
set print static-members on
set print vtbl on
set print demangle on
set demangle-style gnu-v3
set print elements 0
set print repeats 0
set print frame-arguments all
set width 0
set height 0

printf "\n============================================================\n"
printf "TCCCompiler DEADLOCK GDB REPORT\n"
printf "============================================================\n\n"

printf "\n\n============================================================\n"
printf "GDB VERSION\n"
printf "============================================================\n\n"
show version

printf "\n\n============================================================\n"
printf "THREAD LIST\n"
printf "============================================================\n\n"
info threads

printf "\n\n============================================================\n"
printf "ALL THREAD BACKTRACES\n"
printf "============================================================\n\n"
thread apply all bt

printf "\n\n============================================================\n"
printf "ALL THREAD FULL BACKTRACES\n"
printf "============================================================\n\n"
thread apply all bt full

printf "\n\n============================================================\n"
printf "ALL THREAD REGISTERS\n"
printf "============================================================\n\n"
thread apply all info registers

printf "\n\n============================================================\n"
printf "ALL THREAD FRAME INFORMATION\n"
printf "============================================================\n\n"
thread apply all info frame

printf "\n\n============================================================\n"
printf "CURRENT THREAD\n"
printf "============================================================\n\n"
thread

printf "\n\n============================================================\n"
printf "SHARED LIBRARIES\n"
printf "============================================================\n\n"
info sharedlibrary

printf "\n\n============================================================\n"
printf "PROCESS INFORMATION\n"
printf "============================================================\n\n"
info proc

printf "\n\n============================================================\n"
printf "PROCESS MAPPINGS\n"
printf "============================================================\n\n"
info proc mappings

printf "\n\n============================================================\n"
printf "PROCESS STATUS\n"
printf "============================================================\n\n"
info proc status

printf "\n\n============================================================\n"
printf "THREADS / LIBTHREAD-DB\n"
printf "============================================================\n\n"
info threads

printf "\n\n============================================================\n"
printf "SIGNALS\n"
printf "============================================================\n\n"
info signals

printf "\n\n============================================================\n"
printf "GDB THREAD INFO\n"
printf "============================================================\n\n"
maint info thread-events

printf "\n\n============================================================\n"
printf "LOADED OBJECT FILES\n"
printf "============================================================\n\n"
info files

printf "\n\n============================================================\n"
printf "END OF GDB REPORT\n"
printf "============================================================\n\n"

detach
quit
EOF


# ------------------------------------------------------------
# Attach GDB and collect everything
# ------------------------------------------------------------

echo "[7/12] ATTACHING GDB..."
echo
echo "IMPORTANT: TCCCompiler will be STOPPED while GDB collects data."
echo

gdb \
    -q \
    -nx \
    -batch \
    -p "$PID" \
    -x "$GDB_SCRIPT" \
    > "$OUTDIR/gdb_report.txt" \
    2>&1

GDB_EXIT=$?

echo "GDB exited with code: $GDB_EXIT"


# ------------------------------------------------------------
# GDB error/warning extraction
# ------------------------------------------------------------

echo "[8/12] Extracting GDB warnings/errors..."

grep -Ei \
    'error|warning|cannot|unable|no symbol|not available|optimized out|unknown' \
    "$OUTDIR/gdb_report.txt" \
    > "$OUTDIR/gdb_warnings.txt" \
    2>/dev/null || true


# ------------------------------------------------------------
# Thread summary
# ------------------------------------------------------------

echo "[9/12] Creating thread summary..."

{
    echo "============================================================"
    echo "THREAD SUMMARY"
    echo "============================================================"
    echo

    echo "Process:"
    echo "$PROGRAM"
    echo

    echo "PID:"
    echo "$PID"
    echo

    echo "Threads:"
    ls -1 "$THREAD_DIR"/thread_*.txt 2>/dev/null \
        | sed 's/.*thread_/TID /' \
        | sed 's/.txt//' \
        || true

    echo
    echo "Kernel wait channels:"
    echo

    for TASK in /proc/"$PID"/task/*; do
        if [ -d "$TASK" ]; then
            TID="$(basename "$TASK")"
            WCHAN="$(cat "$TASK/wchan" 2>/dev/null || echo unknown)"
            printf "TID %-10s %s\n" "$TID" "$WCHAN"
        fi
    done

} > "$OUTDIR/thread_summary.txt"


# ------------------------------------------------------------
# System information
# ------------------------------------------------------------

echo "[10/12] Collecting system information..."

{
    echo "============================================================"
    echo "SYSTEM INFORMATION"
    echo "============================================================"
    echo

    echo "Kernel:"
    uname -a

    echo
    echo "Distribution:"
    cat /etc/os-release 2>/dev/null || true

    echo
    echo "CPU:"
    lscpu 2>/dev/null || true

    echo
    echo "Memory:"
    free -h 2>/dev/null || true

} > "$OUTDIR/system.txt"


# ------------------------------------------------------------
# GDB executable / binary information
# ------------------------------------------------------------

echo "[11/12] Collecting executable information..."

EXE="$(readlink -f "/proc/$PID/exe" 2>/dev/null || true)"

if [ -n "$EXE" ] && [ -f "$EXE" ]; then

    {
        echo "============================================================"
        echo "EXECUTABLE"
        echo "============================================================"
        echo

        echo "Path:"
        echo "$EXE"

        echo
        echo "File:"
        file "$EXE"

        echo
        echo "ELF information:"
        readelf -h "$EXE" 2>/dev/null || true

        echo
        echo "Build ID:"
        readelf -n "$EXE" 2>/dev/null \
            | grep -A2 -i 'Build ID' \
            || true

    } > "$OUTDIR/executable.txt"

fi


# ------------------------------------------------------------
# Final index
# ------------------------------------------------------------

echo "[12/12] Creating report index..."

cat > "$OUTDIR/README.txt" <<EOF
TCCCompiler DEADLOCK DIAGNOSTIC REPORT
======================================

Generated:
$TIMESTAMP

Program:
$PROGRAM

PID:
$PID


IMPORTANT FILES
===============

gdb_report.txt
    Main GDB report.

    Contains:
      - every thread
      - every thread's backtrace
      - full backtraces
      - local variables
      - function arguments
      - registers
      - loaded shared libraries
      - process mappings
      - executable information
      - GDB information


thread_summary.txt
    Quick summary of all threads and their kernel wait channels.


threads/
    Individual kernel-level information for every thread.

    Each file contains:
      - thread status
      - scheduler information
      - wait channel
      - kernel stack
      - current syscall


process.txt
    Process status and command line.


memory_maps.txt
    Complete /proc memory map.


memory_smaps.txt
    Detailed memory mappings, when available.


file_descriptors.txt
    Open file descriptors and what they point to.


limits.txt
    Process resource limits.


system.txt
    Kernel, distribution, CPU and memory information.


executable.txt
    Information about the TCCCompiler executable.


gdb_warnings.txt
    GDB warnings/errors extracted from the main report.


gdb_commands.txt
    Exact GDB commands used to produce the report.

EOF


# ------------------------------------------------------------
# Compress everything
# ------------------------------------------------------------

echo
echo "Creating archive..."

tar -czf "${OUTDIR}.tar.gz" "$OUTDIR"

echo
echo "============================================================"
echo " COMPLETE"
echo "============================================================"
echo
echo "Directory:"
echo "  $OUTDIR"
echo
echo "Archive:"
echo "  ${OUTDIR}.tar.gz"
echo
echo "Main report:"
echo "  $OUTDIR/gdb_report.txt"
echo
echo "You can give the entire directory/archive to an AI."
echo
