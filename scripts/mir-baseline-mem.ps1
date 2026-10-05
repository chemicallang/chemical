# mir-baseline-mem.ps1
#
# Helper for scripts/mir-baseline.sh. Runs a command line, captures its output to
# -Log, and samples the peak working set of the Chemical compiler process(es)
# (TCCCompiler / Compiler) while the command runs.
#
# Emits a single JSON line: {"exit":N,"peak_bytes":N|null,"ms":N}
param(
  [Parameter(Mandatory = $true)][string]$CommandLine,
  [string]$Log = "$env:TEMP\mir-baseline-mem.log"
)

$ErrorActionPreference = 'SilentlyContinue'

$launcherArgs = @(
  '-NoProfile', '-NonInteractive', '-Command',
  "& { $CommandLine } *> `"$Log`""
)

$sw = [System.Diagnostics.Stopwatch]::StartNew()
$proc = Start-Process -FilePath 'powershell' -ArgumentList $launcherArgs -PassThru -NoNewWindow
$max = 0

while (-not $proc.HasExited) {
  foreach ($name in @('TCCCompiler', 'Compiler')) {
    Get-Process -Name $name -ErrorAction SilentlyContinue | ForEach-Object {
      try {
        $_.Refresh()
        if ($_.PeakWorkingSet64 -gt $max) { $max = $_.PeakWorkingSet64 }
      } catch { }
    }
  }
  Start-Sleep -Milliseconds 150
}
$proc.WaitForExit()
$sw.Stop()

$peak = if ($max -gt 0) { $max } else { $null }
[PSCustomObject]@{
  exit       = $proc.ExitCode
  peak_bytes = $peak
  ms         = $sw.ElapsedMilliseconds
} | ConvertTo-Json -Compress
