<#
.SYNOPSIS
    Copy a build's plugin, a folder of its own, into the game; or take it out.

.DESCRIPTION
    After `.\tools\build.ps1 -Preset <preset>`, puts the plugin's folder,
    build\<preset>\daolib\plugins\photocam\ (photocam.dll and its .pdb), under
    bin_ship\daolib\plugins. The game must
    not be running from that folder: it holds those files open. DAOLib's loader
    (dinput8.dll) and its address files are DAOLib's to put there (its own deploy.ps1, or a
    release of it); other plugins in the folder are left as they are.

    The keys are the game's to keep, in KeyBindings.ini with its own; a deploy touches none.

    -Remove deletes the plugin's folder. The log is in Documents\BioWare\Dragon Age\daolib\
    and stays. A folder that is a link, or has one anywhere in it (a junction, a symbolic
    link), is not deleted: a recursive delete goes through one and empties what it points at.

.EXAMPLE
    .\tools\deploy.ps1                 # debug build into the GOG install
    .\tools\deploy.ps1 -Preset release
    .\tools\deploy.ps1 -Remove
#>
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release')]
    [string] $Preset = 'debug',
    [switch] $Remove,
    [string] $Game = 'C:\Program Files (x86)\GOG Galaxy\Games\Dragon Age Origins\bin_ship'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'console.ps1')
$repo = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path (Join-Path $Game 'DAOrigins.exe'))) { throw "No DAOrigins.exe in $Game" }
# The game holds its DLLs open, and only one running from this folder holds this folder's.
# Another has the same process name (another install; a peer's copy), so the running one is
# asked where it runs from. One whose path cannot be read is taken for this folder's.
$exe = (Resolve-Path (Join-Path $Game 'DAOrigins.exe')).Path
foreach ($running in @(Get-Process -Name DAOrigins -ErrorAction SilentlyContinue)) {
    $from = $running.Path
    if (-not $from -or [string]::Equals($from, $exe, [StringComparison]::OrdinalIgnoreCase)) {
        $where = if ($from) { $from } else { 'a path that cannot be read' }
        throw "DAOrigins.exe is running from $where (process $($running.Id)); close it first"
    }
    Show-Line "a game is running from $from (process $($running.Id)); it does not hold this folder's files"
}

$plugins = Join-Path $Game 'daolib\plugins'
$pluginFolder = Join-Path $plugins 'photocam'

function Find-ReparsePoint([string] $Path) {
    # The folder itself or anything in it that is a link: a junction, a symbolic link.
    $top = Get-Item -LiteralPath $Path -Force
    if ($top.Attributes -band [IO.FileAttributes]::ReparsePoint) { return $top.FullName }
    foreach ($item in Get-ChildItem -LiteralPath $Path -Recurse -Force -Attributes ReparsePoint) {
        return $item.FullName
    }
    return $null
}

function Remove-PluginFolder {
    # (ShouldProcess because PSScriptAnalyzer asks it of a function that removes; the
    # script has no -WhatIf, so the answer is always yes.)
    [CmdletBinding(SupportsShouldProcess)]
    param()
    if (-not (Test-Path $pluginFolder)) { return }
    $link = Find-ReparsePoint $pluginFolder
    if ($link) { throw "$link is a link; the plugin's folder is not deleted through one. Remove it by hand." }
    if ($PSCmdlet.ShouldProcess($pluginFolder, 'Remove the folder')) {
        Remove-Item -LiteralPath $pluginFolder -Recurse -Force
    }
}

if ($Remove) {
    Remove-PluginFolder
    Show-Line "removed $pluginFolder" -Colour Green
    return
}

$built = Join-Path $repo "build\$Preset\daolib\plugins\photocam"
if (-not (Test-Path (Join-Path $built 'photocam.dll'))) {
    throw "No photocam.dll in $built; run .\tools\build.ps1 -Preset $Preset first"
}
# The movies are built outside CMake (tools\movie.py) and copied beside the dll by the build; without
# them photo cam has no panel and no camera in the top bar.
foreach ($movie in 'photocam.swf', 'photocam_navbar.swf') {
    if (-not (Test-Path (Join-Path $built $movie))) {
        Show-Line "no $movie in $built`: run python tools\movie.py, then build again" -Colour Yellow
    }
}
if (-not (Test-Path (Join-Path $Game 'dinput8.dll'))) {
    Show-Line "no dinput8.dll beside DAOrigins.exe: DAOLib's loader is not installed, and nothing loads the plugin" -Colour Yellow
}
Remove-PluginFolder
New-Item -ItemType Directory -Force $plugins | Out-Null
Copy-Item -LiteralPath $built -Destination $pluginFolder -Recurse
Show-Line "deployed $built to $pluginFolder" -Colour Green
