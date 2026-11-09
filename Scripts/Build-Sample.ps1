[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.9',
    [ValidatePattern('^5\.9\.\d+$')]
    [string]$ExpectedEngineVersion = '5.9.0',
    [ValidateSet('Validate', 'Upgrade', 'Lighting', 'Package', 'All')]
    [string]$Phase = 'Validate',
    [switch]$RunAutomation
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Unreal reads the raw Windows command line. Keep embedded value quotes intact
# on PowerShell 7 as well as Windows PowerShell 5.1 (script scope only).
if (Get-Variable -Name PSNativeCommandArgumentPassing -ErrorAction SilentlyContinue) {
    $PSNativeCommandArgumentPassing = 'Legacy'
}

function Invoke-CheckedNative {
    param(
        [Parameter(Mandatory)][string]$Label,
        [Parameter(Mandatory)][string]$Executable,
        [Parameter(Mandatory)][string[]]$ArgumentList
    )
    Write-Host "`n== $Label ==" -ForegroundColor Cyan
    & $Executable @ArgumentList
    $result = $LASTEXITCODE
    if ($result -ne 0) {
        throw "$Label failed with exit code $result. See console output and Saved/Logs."
    }
}

function Assert-AutomationReport {
    param(
        [Parameter(Mandatory)][string]$ReportPath,
        [Parameter(Mandatory)][datetime]$StartedUtc
    )
    if (-not (Test-Path -LiteralPath $ReportPath -PathType Leaf)) {
        throw "Automation did not create a report: $ReportPath"
    }
    $reportFile = Get-Item -LiteralPath $ReportPath
    if ($reportFile.LastWriteTimeUtc -lt $StartedUtc) {
        throw "Automation report predates this run: $ReportPath"
    }
    $report = Get-Content -LiteralPath $ReportPath -Raw | ConvertFrom-Json
    foreach ($field in @('succeeded', 'succeededWithWarnings', 'failed', 'notRun', 'inProcess', 'tests')) {
        if ($field -notin $report.PSObject.Properties.Name) {
            throw "Automation report is missing '$field': $ReportPath"
        }
    }
    foreach ($field in @('succeeded', 'succeededWithWarnings', 'failed', 'notRun', 'inProcess')) {
        $count = 0
        if (-not [int]::TryParse([string]$report.$field, [ref]$count) -or $count -lt 0) {
            throw "Automation report has an invalid '$field' count: $ReportPath"
        }
    }
    $passed = [int]$report.succeeded + [int]$report.succeededWithWarnings
    if ($null -eq $report.tests -or @($report.tests).Count -eq 0 -or $passed -eq 0 -or
        [int]$report.failed -ne 0 -or [int]$report.notRun -ne 0 -or [int]$report.inProcess -ne 0) {
        throw "Automation report is incomplete or failed: passed=$passed, failed=$($report.failed), notRun=$($report.notRun), inProcess=$($report.inProcess). See $ReportPath"
    }
    foreach ($expectedTest in @('ActionRPG.Port.GASCooldownAndPossession', 'ActionRPG.Port.GoblinVariantInitialization')) {
        $matchingTests = @($report.tests | Where-Object { $_.fullTestPath -eq $expectedTest -and $_.state -eq 'Success' })
        if ($matchingTests.Count -ne 1) {
            throw "Required test did not run successfully: $expectedTest. See $ReportPath"
        }
    }
    Write-Host "Automation report verified: $passed passed ($($report.succeededWithWarnings) with warnings)."
}

$projectRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..')).Replace('\', '/')
$EngineRoot = [IO.Path]::GetFullPath($EngineRoot).Replace('\', '/')
$projectFile = "$projectRoot/ActionRPG.uproject"
$buildTool = "$EngineRoot/Engine/Build/BatchFiles/Build.bat"
$editor = "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"
$uat = "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat"
$versionFile = "$EngineRoot/Engine/Build/Build.version"

foreach ($required in @($projectFile, $buildTool, $editor, $uat, $versionFile)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required file missing: $required"
    }
}
$version = Get-Content -LiteralPath $versionFile -Raw | ConvertFrom-Json
$actualVersion = "$($version.MajorVersion).$($version.MinorVersion).$($version.PatchVersion)"
if ($actualVersion -ne $ExpectedEngineVersion) {
    throw "This preview workflow expects UE $ExpectedEngineVersion; EngineRoot contains $actualVersion. Use the 5.8 main branch for the stable community baseline."
}

$savedRoot = "$projectRoot/Saved"
$logRoot = "$savedRoot/Logs"
New-Item -ItemType Directory -Path $logRoot -Force | Out-Null
$version | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath "$savedRoot/EngineBuildVersion.json" -Encoding UTF8
Write-Host "Preview engine: $actualVersion, changelist $($version.Changelist), branch $($version.BranchName)"
$editorArguments = @($projectFile, '-unattended', '-nop4', '-nosplash', '-stdout', '-FullStdOutLogOutput', '-UTF8Output')

# Run from this project so relative Unreal paths consistently refer to this copy.
Push-Location -LiteralPath $projectRoot
try {
    Invoke-CheckedNative -Label 'Build ActionRPGEditor (Win64 Development)' -Executable $buildTool -ArgumentList @(
        'ActionRPGEditor', 'Win64', 'Development', "-Project=`"$projectFile`"", '-WaitMutex', '-NoHotReloadFromIDE'
    )

    if ($Phase -in @('Upgrade', 'All')) {
        Invoke-CheckedNative -Label 'Repair sample compatibility assets' -Executable $editor -ArgumentList (
            $editorArguments + @('-run=RPGUpgradeSample', '-NullRHI', "-Abslog=`"$logRoot/Port-Upgrade.log`"")
        )
        Invoke-CheckedNative -Label "Resave project packages with UE $actualVersion" -Executable $editor -ArgumentList (
            $editorArguments + @('-run=ResavePackages', '-ProjectOnly', '-IgnoreChangelist', '-NullRHI', "-Abslog=`"$logRoot/Port-Resave.log`"")
        )
    }

    if ($Phase -in @('Lighting', 'All')) {
        Write-Host 'Production lighting rebuild can take a long time and writes map/build-data assets.'
        Invoke-CheckedNative -Label 'Build production lighting for entry maps and streamed content' -Executable $editor -ArgumentList (
            $editorArguments + @(
                '-run=ResavePackages', '-BuildLighting', '-Quality=Production', '-AllowCommandletRendering',
                '-Map=/Game/Maps/ActionRPG_Main+/Game/Maps/ActionRPG_P',
                '-MapsOnly', '-ProjectOnly', '-IgnoreChangelist', "-Abslog=`"$logRoot/Port-Lighting.log`""
            )
        )
    }

    if ($Phase -in @('Validate', 'All')) {
        # CompileAllBlueprints compares exact object paths, not directory prefixes.
        # Its AllowListFile path is relative to FPaths::ProjectDir(), so keep it relative.
        $contentRoot = "$projectRoot/Content/"
        $allowList = @(Get-ChildItem -LiteralPath $contentRoot -Filter '*.uasset' -File -Recurse |
            Sort-Object FullName | ForEach-Object {
                $relativeAsset = $_.FullName.Replace('\', '/').Substring($contentRoot.Length)
                $packagePath = '/Game/' + $relativeAsset.Substring(0, $relativeAsset.Length - '.uasset'.Length)
                $packagePath + '.' + $_.BaseName
            })
        if ($allowList.Count -eq 0) {
            throw 'No Content/*.uasset files found; refusing an empty Blueprint allow list.'
        }
        [IO.File]::WriteAllLines("$savedRoot/BlueprintAllowList.txt", [string[]]$allowList, [Text.UTF8Encoding]::new($false))
        Write-Host "Generated Saved/BlueprintAllowList.txt ($($allowList.Count) asset object paths)."
        Invoke-CheckedNative -Label 'Compile all project Blueprints' -Executable $editor -ArgumentList (
            $editorArguments + @('-run=CompileAllBlueprints', '-AllowListFile=Saved/BlueprintAllowList.txt', '-NullRHI', "-Abslog=`"$logRoot/Port-Blueprints.log`"")
        )

        if ($RunAutomation) {
            $automationReport = "$savedRoot/Automation/index.json"
            # Remove the old index first so a zero-test/crashed run cannot reuse a
            # previous success. Check the new file's write time against this run.
            if (Test-Path -LiteralPath $automationReport -PathType Leaf) {
                Remove-Item -LiteralPath $automationReport
            }
            $automationStartedUtc = [datetime]::UtcNow
            Invoke-CheckedNative -Label 'Run ActionRPG port automation tests' -Executable $editor -ArgumentList (
                $editorArguments + @(
                    '-NullRHI', '-nosound', '-ExecCmds="Automation RunTests ActionRPG.Port;Quit"',
                    '-TestExit="Automation Test Queue Empty"', "-ReportExportPath=`"$savedRoot/Automation`"",
                    "-Abslog=`"$logRoot/Port-Automation.log`""
                )
            )
            Assert-AutomationReport -ReportPath $automationReport -StartedUtc $automationStartedUtc
        }
    }

    if ($Phase -in @('Package', 'All')) {
        Invoke-CheckedNative -Label 'Build, cook, and package Windows Development' -Executable $uat -ArgumentList @(
            'BuildCookRun', "-project=`"$projectFile`"", '-noP4', '-unattended', '-platform=Win64',
            '-clientconfig=Development', '-build', '-cook', '-stage', '-pak', '-archive',
            "-archivedirectory=`"$projectRoot/Artifacts/Windows`"", '-utf8output'
        )
    }

    Write-Host "`n$Phase completed successfully." -ForegroundColor Green
}
finally {
    Pop-Location
}
