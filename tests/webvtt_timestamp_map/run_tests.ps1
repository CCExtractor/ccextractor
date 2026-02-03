# WebVTT X-TIMESTAMP-MAP Test Script (PowerShell)
# Tests that CCExtractor always includes X-TIMESTAMP-MAP header in WebVTT output

param(
    [Parameter(Mandatory=$false)]
    [string]$CCExtractorPath = ".\ccextractor.exe"
)

$TestDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$OutputDir = Join-Path $TestDir "output"
$Passed = 0
$Failed = 0

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "WebVTT X-TIMESTAMP-MAP Test Suite" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "CCExtractor: $CCExtractorPath"
Write-Host ""

# Check if ccextractor exists
if (-not (Test-Path $CCExtractorPath)) {
    Write-Host "ERROR: CCExtractor not found at $CCExtractorPath" -ForegroundColor Red
    Write-Host "Usage: .\run_tests.ps1 -CCExtractorPath 'C:\path\to\ccextractor.exe'"
    exit 1
}

# Create output directory
New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
Remove-Item "$OutputDir\*.vtt" -ErrorAction SilentlyContinue

function Run-Test {
    param(
        [string]$TestName,
        [string]$InputFile,
        [string]$ExtraArgs = ""
    )
    
    $OutputFile = Join-Path $OutputDir "$TestName.vtt"
    Write-Host -NoNewline "Test: $TestName... "
    
    # Run CCExtractor
    $args = @($InputFile, "-out=webvtt", "-o", $OutputFile)
    if ($ExtraArgs) {
        $args += $ExtraArgs.Split(" ")
    }
    
    & $CCExtractorPath @args 2>&1 | Out-Null
    
    # Check if output file exists
    if (-not (Test-Path $OutputFile)) {
        Write-Host "FAILED (no output file)" -ForegroundColor Red
        $script:Failed++
        return
    }
    
    # Read file content
    $content = Get-Content $OutputFile
    
    # Check for WEBVTT header on line 1
    if ($content[0] -ne "WEBVTT") {
        Write-Host "FAILED (line 1 is not WEBVTT)" -ForegroundColor Red
        $script:Failed++
        return
    }
    
    # Check for X-TIMESTAMP-MAP on line 2
    if (-not ($content[1] -match "^X-TIMESTAMP-MAP=")) {
        Write-Host "FAILED (line 2 missing X-TIMESTAMP-MAP)" -ForegroundColor Red
        $script:Failed++
        return
    }
    
    # Check for exactly one X-TIMESTAMP-MAP header
    $count = ($content | Select-String "X-TIMESTAMP-MAP").Count
    if ($count -ne 1) {
        Write-Host "FAILED (found $count X-TIMESTAMP-MAP headers, expected 1)" -ForegroundColor Red
        $script:Failed++
        return
    }
    
    Write-Host "PASSED" -ForegroundColor Green -NoNewline
    Write-Host " [$($content[1])]"
    $script:Passed++
}

# Create a minimal empty TS file for testing
Write-Host "Creating test files..."
$emptyTs = Join-Path $TestDir "empty_test.ts"
$bytes = New-Object byte[] (188 * 100)
[System.IO.File]::WriteAllBytes($emptyTs, $bytes)

# Test 1: Empty TS file (no subtitles)
Run-Test -TestName "empty_ts_no_subs" -InputFile $emptyTs

# Test 2: Empty TS with invalid datapid
Run-Test -TestName "invalid_datapid" -InputFile $emptyTs -ExtraArgs "--datapid 9999"

# Test 3: Real sample if available
$sampleTs = Join-Path $TestDir "sample.ts"
if (Test-Path $sampleTs) {
    Run-Test -TestName "real_sample" -InputFile $sampleTs
}

# Cleanup
Remove-Item $emptyTs -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "Results: $Passed passed, $Failed failed" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

if ($Failed -gt 0) {
    exit 1
}
exit 0
