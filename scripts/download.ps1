# install.ps1
$ErrorActionPreference = "Stop"

# Settings
$VERSION = if ($env:VERSION) { $env:VERSION } else { "latest" }
$VARIANT = if ($env:VARIANT) { $env:VARIANT } else { "" }
$OWNER = "chemicallang"
$REPO = "chemical"

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
            $VERSION = "v0.0.30"
            Write-Host "Warning: API failed, falling back to $VERSION"
        }
    }
}

# 3. Determine Candidate Filenames logic
$candidates = New-Object System.Collections.Generic.List[string]
if ($VARIANT) {
    $candidates.Add("${platform}-${arch}-${VARIANT}.zip")
}
$candidates.Add("${platform}-${arch}.zip")

if ($arch -ne "x64") {
    if ($VARIANT) {
        $candidates.Add("${platform}-x64-${VARIANT}.zip")
    }
    $candidates.Add("${platform}-x64.zip")
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

Write-Host "Done! Chemical installed to $installDir"
