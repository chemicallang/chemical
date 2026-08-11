# install.ps1 — usage:  .\download.ps1 [-DebugBuild]
#   -DebugBuild   download the DEBUG build (full debug symbols, published to
#                 chemicallang/chemical-debug by the Build & Release workflow
#                 when deploy_debug is enabled) instead of the stable release.
#                 Debug releases are tagged with the source commit SHA; set
#                 $env:VERSION to a commit SHA to pick a specific debug build.
# Env: VERSION, VARIANT, ARCH_OVERRIDE, DEBUG (1/true = same as -DebugBuild)
param([switch]$DebugBuild)
$ErrorActionPreference = "Stop"

# Settings
$VERSION = if ($env:VERSION) { $env:VERSION } else { "latest" }
$VARIANT = if ($env:VARIANT) { $env:VARIANT } else { "" }
if ($env:DEBUG -and $env:DEBUG -ne "0") { $DebugBuild = $true }
$OWNER = "chemicallang"
if ($DebugBuild) {
    $REPO = "chemical-debug"
} else {
    $REPO = "chemical"
}

# 1. Detect Architecture
function Detect-Arch {
    if ($env:ARCH_OVERRIDE) { return $env:ARCH_OVERRIDE }
    $m = $env:PROCESSOR_ARCHITECTURE
    switch -Regex ($m) {
        "AMD64" { return "x64" }
        "ARM64" { return "arm64" }
        "x86"   { return "x64" } # treat 32-bit as x64 fallback
        Default { return "x64" }
    }
}

$arch = Detect-Arch
$platform = "windows"

# 2. Get Latest Version if needed
if ($VERSION -eq "latest" -or !$VERSION) {
    if ($DebugBuild) {
        Write-Host "Checking for latest debug build..."
        # Debug releases are tagged with commit SHAs (not version tags), so
        # query the GitHub API for releases newest-first and take the first
        # one that actually has assets.
        try {
            $url = "https://api.github.com/repos/$OWNER/$REPO/releases?per_page=10"
            $rels = Invoke-RestMethod -Uri $url -ErrorAction Stop
            foreach ($r in $rels) {
                if ($r.assets -and $r.assets.Count -gt 0) {
                    $VERSION = $r.tag_name
                    break
                }
            }
        } catch {
            Write-Host "Warning: could not query $OWNER/$REPO releases: $_"
        }
        if ($VERSION -eq "latest" -or !$VERSION) {
            Write-Error "No debug builds found in $OWNER/$REPO (was the Build & Release workflow run with deploy_debug enabled?)"
            exit 2
        }
        Write-Host "Latest debug build detected: $VERSION"
    } else {
        Write-Host "Checking for latest release..."
        try {
            # Try git if available
            if (Get-Command git -ErrorAction SilentlyContinue) {
                $tags = git ls-remote --tags --sort="v:refname" "https://github.com/$OWNER/$REPO.git"
                if ($tags) {
                    # Get the last line, extract tag name
                    $lastTagLine = $tags[-1]
                    if ($lastTagLine -match "refs/tags/(.*)") {
                        $VERSION = $Matches[1]
                        Write-Host "Latest version detected via git: $VERSION"
                    }
                }
            }
        } catch {}

        if ($VERSION -eq "latest") {
            try {
                # Try GitHub Tags API
                $url = "https://api.github.com/repos/$OWNER/$REPO/tags"
                $resp = Invoke-RestMethod -Uri $url
                if ($resp) {
                    $VERSION = $resp[0].name
                    Write-Host "Latest version detected via API: $VERSION"
                }
            } catch {
                $VERSION = "v0.0.32"
                Write-Host "Warning: API failed, falling back to $VERSION"
            }
        }
    }
}

# 3. Determine Candidate Filenames logic
#    In -DebugBuild mode every asset is suffixed with -debug
#    (e.g. windows-x64-tcc-debug.zip) and lives in the chemical-debug repo.
$candidates = New-Object System.Collections.Generic.List[string]
if ($DebugBuild) {
    if ($VARIANT) {
        $candidates.Add("${platform}-${arch}-${VARIANT}-debug.zip")
    }
    $candidates.Add("${platform}-${arch}-debug.zip")
} else {
    if ($VARIANT) {
        $candidates.Add("${platform}-${arch}-${VARIANT}.zip")
    }
    $candidates.Add("${platform}-${arch}.zip")
}

if ($arch -ne "x64") {
    if ($DebugBuild) {
        if ($VARIANT) {
            $candidates.Add("${platform}-x64-${VARIANT}-debug.zip")
        }
        $candidates.Add("${platform}-x64-debug.zip")
    } else {
        if ($VARIANT) {
            $candidates.Add("${platform}-x64-${VARIANT}.zip")
        }
        $candidates.Add("${platform}-x64.zip")
    }
}

# 4. Setup Install Directory
$installDir = if ($env:USERPROFILE) { Join-Path $env:USERPROFILE ".chemical" } else { Join-Path $env:TEMP ".chemical" }
if (!(Test-Path $installDir)) { New-Item -ItemType Directory -Path $installDir -Force }

# 5. Download and Extract
$baseUrl = "https://github.com/$OWNER/$REPO/releases/download/$VERSION"
$found = $false

foreach ($assetName in $candidates) {
    $downloadUrl = "$baseUrl/$assetName"
    $zipPath = Join-Path $env:TEMP "chemical_$($assetName)"
    
    Write-Host "Checking $downloadUrl ..."
    try {
        Invoke-WebRequest -Uri $downloadUrl -OutFile $zipPath -ErrorAction Stop
        Write-Host "Found $assetName -> downloading..."
        $found = $true
        
        Write-Host "Extracting to $installDir ..."
        # Clean old contents
        if (Test-Path $installDir) {
            Get-ChildItem -Path $installDir | Remove-Item -Recurse -Force
        }
        
        $extractPath = Join-Path $env:TEMP "chemical_extract_$(Get-Random)"
        Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force
        
        $items = Get-ChildItem $extractPath
        if ($items.Count -eq 1 -and $items[0].PSIsContainer) {
            # Single top-level dir, move its contents
            Move-Item -Path "$(Join-Path $extractPath $items[0].Name)\*" -Destination $installDir -Force
        } else {
            Move-Item -Path "$extractPath\*" -Destination $installDir -Force
        }
        
        Remove-Item $zipPath -Force
        Remove-Item $extractPath -Recurse -Force
        break
    } catch {
        if ($found) {
            # If $found is true, it means Invoke-WebRequest succeeded but something else failed
            Write-Error "ERROR: Failed during extraction/installation of $assetName : $_"
            exit 1
        }
        Write-Host "Not found: $assetName"
    }
}

if (!$found) {
    Write-Error "ERROR: no release asset found for any of: $($candidates -join ', ')"
    exit 2
}

# 6. Finalize
$exePath = Join-Path $installDir "chemical.exe"
if (Test-Path $exePath) {
    Write-Host "Running configure..."
    & $exePath --configure
}

# Update the current session so `chemical` works right now without reopening the terminal.
if ($env:PATH -notlike "*$installDir*") {
    $env:PATH = "$installDir;$env:PATH"
}

Write-Host ""
if ($DebugBuild) {
    Write-Host "Done! Chemical (DEBUG build, from $OWNER/$REPO) installed to $installDir"
} else {
    Write-Host "Done! Chemical installed to $installDir"
}
