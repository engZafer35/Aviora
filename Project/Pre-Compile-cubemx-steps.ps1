# Post-CubeMX patches for Driver/StmCubeIDE (Windows; run from any cwd).
# Linux/macOS: Project/Pre-Compile-cubemx-steps.sh
$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$AvioraRoot = (Resolve-Path (Join-Path $ScriptDir "..")).Path

$MainH = Join-Path $AvioraRoot "Driver\StmCubeIDE\Inc\main.h"
$SrcDir = Join-Path $AvioraRoot "Driver\StmCubeIDE\Src"
$MainC = Join-Path $SrcDir "main.c"
$MainStm = Join-Path $SrcDir "main_stm.c"
$FreeRtosSrc = Join-Path $AvioraRoot "Middleware\MiddComm\Midd_OS\inc\FreeRTOSConfig_notInclude"
$FreeRtosDst = Join-Path $AvioraRoot "Driver\StmCubeIDE\Inc\FreeRTOSConfig.h"

$IncludeLine = '#include "McuSmallHelperFunc.h"'
$UserIncludesMark = '/* USER CODE BEGIN Includes */'

function Write-PreCompileUtf8 {
    param([string]$Path, [string]$Text)
    $utf8NoBom = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($Path, $Text, $utf8NoBom)
}

function Die {
    param([string]$Message)
    Write-Error "pre-compile-cubemx-steps: $Message"
}

function Patch-MainH {
    if (-not (Test-Path -LiteralPath $MainH)) {
        Die "missing $MainH"
    }

    $text = [System.IO.File]::ReadAllText($MainH)
    if ($text.Contains($IncludeLine)) {
        Write-Host "main.h: $IncludeLine already present"
        return
    }

    if (-not $text.Contains($UserIncludesMark)) {
        Die "main.h: marker not found: $UserIncludesMark"
    }

    $lines = $text -split "`r?`n", -1
    $out = New-Object System.Collections.Generic.List[string]
    $inserted = $false
    foreach ($line in $lines) {
        $out.Add($line)
        if (-not $inserted -and $line -eq $UserIncludesMark) {
            $out.Add($IncludeLine)
            $inserted = $true
        }
    }

    if (-not $inserted) {
        Die "main.h: failed to insert include"
    }

    $newText = ($out -join [Environment]::NewLine)
    if ($text.EndsWith("`n") -or $text.EndsWith("`r`n")) {
        $newText += [Environment]::NewLine
    }
    Write-PreCompileUtf8 -Path $MainH -Text $newText
    Write-Host "main.h: added $IncludeLine"
}

function Rename-MainC {
    if (-not (Test-Path -LiteralPath $MainC)) {
        if (Test-Path -LiteralPath $MainStm) {
            Write-Host "Src: main.c absent, main_stm.c ok"
            return
        }
        Die "missing $MainC and $MainStm"
    }

    if (Test-Path -LiteralPath $MainStm) {
        Die "both $MainC and $MainStm exist; resolve manually"
    }

    Move-Item -LiteralPath $MainC -Destination $MainStm
    Write-Host "Src: renamed main.c -> main_stm.c"
}

function Patch-MainStmC {
    if (-not (Test-Path -LiteralPath $MainStm)) {
        Die "missing $MainStm"
    }

    $text = [System.IO.File]::ReadAllText($MainStm)
    $changed = $false

    if ($text -match 'HAL_TIM_PeriodElapsedCallback_notused') {
        Write-Host "main_stm.c: HAL_TIM_PeriodElapsedCallback already patched"
    } else {
        $newText = [regex]::Replace(
            $text,
            '\bHAL_TIM_PeriodElapsedCallback\b',
            'HAL_TIM_PeriodElapsedCallback_notused'
        )
        if ($newText -eq $text) {
            Die "main_stm.c: HAL_TIM_PeriodElapsedCallback not found"
        }
        $text = $newText
        $changed = $true
        Write-Host "main_stm.c: renamed HAL_TIM_PeriodElapsedCallback"
    }

    if ($text -match 'int main_notused\s*\(\s*void\s*\)') {
        Write-Host "main_stm.c: main already patched"
    } else {
        $newText = $text -replace 'int main\s*\(\s*void\s*\)', 'int main_notused(void)'
        if ($newText -eq $text) {
            Die "main_stm.c: int main(void) not found"
        }
        $text = $newText
        $changed = $true
        Write-Host "main_stm.c: renamed int main(void)"
    }

    if ($changed) {
        Write-PreCompileUtf8 -Path $MainStm -Text $text
    }
}

function Install-FreeRtosConfig {
    if (-not (Test-Path -LiteralPath $FreeRtosSrc)) {
        Die "missing $FreeRtosSrc"
    }

    $dstDir = Split-Path -Parent $FreeRtosDst
    if (-not (Test-Path -LiteralPath $dstDir)) {
        New-Item -ItemType Directory -Path $dstDir -Force | Out-Null
    }

    if ((Test-Path -LiteralPath $FreeRtosDst) -and
        ((Get-FileHash -LiteralPath $FreeRtosSrc).Hash -eq (Get-FileHash -LiteralPath $FreeRtosDst).Hash)) {
        Write-Host "FreeRTOSConfig.h: already matches FreeRTOSConfig_notInclude"
        return
    }

    if (Test-Path -LiteralPath $FreeRtosDst) {
        Remove-Item -LiteralPath $FreeRtosDst -Force
    }
    Copy-Item -LiteralPath $FreeRtosSrc -Destination $FreeRtosDst -Force
    Write-Host "FreeRTOSConfig.h: copied from FreeRTOSConfig_notInclude"
}

Patch-MainH
Rename-MainC
Patch-MainStmC
Install-FreeRtosConfig
Write-Host "pre-compile-cubemx-steps: done"
