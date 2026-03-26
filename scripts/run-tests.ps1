# run-tests.ps1
# Copyright (c) Chemical Language Foundation 2026.
#
# Clone the chemical repo (shallow), keep only lang/tests, then run or compile
# the test suite using the `chemical` binary already on PATH.
#
# Usage:
#   .\run-tests.ps1 [--tag <tag>] [--no-run] [--output|-o <file>] [<extra args>]
#
# Options:
#   --tag <tag>       Clone at this specific git tag (default: latest)
#   --no-run          Compile to an executable instead of running directly
#   --output/-o <f>   Output executable name (only meaningful with --no-run)
#   Extra args are forwarded to the underlying `chemical` command.

$ErrorActionPreference = "Stop"

# ── Defaults ──────────────────────────────────────────────────────────────────
$RepoUrl    = "https://github.com/chemicallang/chemical"
$Tag        = ""
$NoRun      = $false
$OutputFile = "tests.exe"
$ExtraArgs  = @()

# ── Argument parsing ──────────────────────────────────────────────────────────
$i = 0
while ($i -lt $args.Count) {
    switch ($args[$i]) {
        "--tag" {
            $i++
            $Tag = $args[$i]
        }
        "--no-run" {
            $NoRun = $true
        }
        { $_ -eq "--output" -or $_ -eq "-o" } {
            $i++
            $OutputFile = $args[$i]
        }
        default {
            $ExtraArgs += $args[$i]
        }
    }
    $i++
}

# ── Clone ─────────────────────────────────────────────────────────────────────
$WorkDir  = Join-Path $env:TEMP ("chemical_tests_" + [System.IO.Path]::GetRandomFileName().Replace(".", ""))
$RepoDir  = Join-Path $WorkDir "chemical"

# Ensure cleanup on exit
function Cleanup {
    if (Test-Path $WorkDir) {
        Remove-Item -Recurse -Force $WorkDir -ErrorAction SilentlyContinue
    }
}
trap { Cleanup; break }

New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null

Write-Host "Cloning $RepoUrl ..."

if ($Tag) {
    git clone --depth 1 --branch $Tag $RepoUrl $RepoDir
} else {
    git clone --depth 1 $RepoUrl $RepoDir
}

# ── Keep only lang/tests ──────────────────────────────────────────────────────
Write-Host "Stripping repo to lang/tests ..."

$TestsSrc  = Join-Path $RepoDir "lang\tests"
$TestsKeep = Join-Path $WorkDir "tests_keep"

if (-not (Test-Path $TestsSrc)) {
    Write-Error "ERROR: lang/tests not found in cloned repo."
    Cleanup
    exit 1
}

# Move lang/tests out, delete everything else, restore
Move-Item -Path $TestsSrc -Destination $TestsKeep
Remove-Item -Recurse -Force $RepoDir
New-Item -ItemType Directory -Path (Join-Path $RepoDir "lang") -Force | Out-Null
Move-Item -Path $TestsKeep -Destination (Join-Path $RepoDir "lang\tests")

# ── Run or compile ────────────────────────────────────────────────────────────
$BuildLab = Join-Path $RepoDir "lang\tests\build.lab"

if (-not (Test-Path $BuildLab)) {
    Write-Error "ERROR: build.lab not found at $BuildLab"
    Cleanup
    exit 1
}

if ($NoRun) {
    Write-Host "Compiling tests -> $OutputFile ..."
    & chemical $BuildLab -o $OutputFile @ExtraArgs
} else {
    Write-Host "Running tests ..."
    & chemical run $BuildLab @ExtraArgs
}

Cleanup
