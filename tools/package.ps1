<#
.SYNOPSIS
    Make the release zip the mod is installed from.

.DESCRIPTION
    After `.	oolsuild.ps1 -Preset release`, writes dist\DAOPhotoCam-<version>.zip,
    laid out as the game's install folder, so it is extracted there (beside bin_ship) as
    DAOLib's own zip is. It holds the plugin's folder and nothing else:

        bin_ship\daolib\plugins\photocam\photocam.dll
        bin_ship\daolib\plugins\photocam\photocam.pdb          so a crash log names the mod's frames
        bin_ship\daolib\plugins\photocam\photocam.swf          the panel's movie
        bin_ship\daolib\plugins\photocam\photocam_navbar.swf   the top bar's camera button
        bin_ship\daolib\plugins\photocam\README.md, LICENSE.txt, VERSION.txt

    No loader and no address files: those are DAOLib's zip, which a game has first.
    DAOTurnBased's tools\package.ps1, copied 2026-10-15, less its override folder.

    The movies are the ones tools\movie.py last built, and this refuses a folder whose
    movie is missing or older than that. VERSION.txt is what the README cannot say: the
    release, and the DAOLib it was built on, which is the tag extern\DAOLib is at, where
    a release holds it (dev/RELEASING.md).

    The version is the project's (CMakeLists.txt). Nothing is copied into the game.

.EXAMPLE
    .\tools\package.ps1                 # from build\release
    .\tools\package.ps1 -Preset debug   # a debug build's, for a trial
#>
[CmdletBinding()]
param(
    [ValidateSet('debug', 'release')]
    [string] $Preset = 'release'
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'console.ps1')
. (Join-Path $PSScriptRoot 'daolib_release.ps1')
$repo = Split-Path -Parent $PSScriptRoot

# A file's SHA-256. Not Get-FileHash: after tools\build.ps1 has taken the compiler's
# environment into the session (as tools\release.ps1 runs the two), Windows PowerShell
# no longer finds that cmdlet's module.
function Get-Sha256([string] $Path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { [BitConverter]::ToString($sha.ComputeHash([IO.File]::ReadAllBytes($Path))) }
    finally { $sha.Dispose() }
}

$built = Join-Path $repo "build\$Preset\daolib\plugins\photocam"
$builtPlugin = Join-Path $built 'photocam.dll'
if (-not (Test-Path $builtPlugin)) { throw "No $builtPlugin; run .\tools\build.ps1 -Preset $Preset first" }
$builtPdb = Join-Path $built 'photocam.pdb'
if (-not (Test-Path $builtPdb)) { throw "No $builtPdb; a crash log names none of the mod's frames without it" }

# The movies, each the one tools\movie.py last built: one built since the build is not
# beside the dll yet, and the plugin and its movies are a pair (tools\deploy.ps1).
$movies = 'photocam.swf', 'photocam_navbar.swf'
foreach ($movie in $movies) {
    $beside = Join-Path $built $movie
    $made = Join-Path $repo "build\ui\$movie"
    if (-not (Test-Path $made)) { throw "No $made; run python tools\movie.py, then the build" }
    if (-not (Test-Path $beside) -or (Get-Sha256 $beside) -ne (Get-Sha256 $made)) {
        throw "$beside is not build\ui\$movie as it is now; run .\tools\build.ps1 -Preset $Preset again"
    }
}

$cmake = Get-Content -Raw (Join-Path $repo 'CMakeLists.txt')
if ($cmake -notmatch 'project\((\w+) VERSION (\d+\.\d+\.\d+)') { throw 'No version in CMakeLists.txt' }
$version = $Matches[2]
$name = "$($Matches[1])-$version"

# The loader and the address files a game has are a DAOLib release's, so VERSION.txt
# says which this was built on. At no tag it is a commit's, which a trial may be and a
# release is not.
$daolib = Join-Path $repo 'extern\DAOLib'
$daolibVersion = Get-DAOLibRelease $daolib
$pluginApi = Get-DAOLibPluginApi $daolib
if ($daolibVersion) {
    $builtOn = "DAOLib $daolibVersion"
}
else {
    $builtOn = "DAOLib at its commit $(git -C $daolib rev-parse --short HEAD), between releases"
    Show-Warn 'extern\DAOLib is at no release''s tag; a release of the mod holds it at one'
}

$out = Join-Path $repo 'dist'
$stage = Join-Path $out $name
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
$plugin = Join-Path $stage 'bin_ship\daolib\plugins\photocam'
New-Item -ItemType Directory -Force $plugin | Out-Null

# By name, not the folder as built: a debug build leaves the linker's .ilk there.
Copy-Item -Force $builtPlugin, $builtPdb $plugin
foreach ($movie in $movies) { Copy-Item -Force (Join-Path $built $movie) $plugin }
Copy-Item -Force (Join-Path $repo 'README.md') $plugin
Copy-Item -Force (Join-Path $repo 'LICENSE') (Join-Path $plugin 'LICENSE.txt')

$versionText = @"
DAOPhotoCam $version
https://github.com/Whocares66-Dev/DAOPhotoCam/releases
Guide: https://whocares66-dev.github.io/DAOPhotoCam/

Built on $builtOn, plugin API $pluginApi
(https://github.com/Whocares66-Dev/DAOLib/releases). The game needs that release of
DAOLib or a later one of the same plugin API: its loader, and its address files. A loader
of another plugin API does not load this plugin, and the logs in
Documents\BioWare\Dragon Age\daolib (loader.log, photocam.log) say so.
"@
Set-Content -Path (Join-Path $plugin 'VERSION.txt') -Value $versionText -Encoding utf8

# What is given out does not say who built it. The linker writes the folder of its
# temporary files into a PDB, which tools\build.ps1 keeps in the build tree for that
# reason (as DAOLib #55); a plugin linked any other way is caught here, and so is any
# other file of the zip that names this user's folder or this user.
$profileFolder = [Environment]::GetFolderPath('UserProfile')
$userName = Split-Path -Leaf $profileFolder
foreach ($file in Get-ChildItem $stage -Recurse -File) {
    $bytes = [Text.Encoding]::GetEncoding(28591).GetString([IO.File]::ReadAllBytes($file.FullName))
    foreach ($word in $profileFolder, "Users\$userName", "Users/$userName") {
        foreach ($written in $word, ($word.ToCharArray() -join "`0")) {
            if ($bytes.IndexOf($written, [StringComparison]::OrdinalIgnoreCase) -ge 0) {
                Remove-Item -Recurse -Force $stage
                throw "$($file.Name) holds the path of this user's folder; build with .\tools\build.ps1 -Preset $Preset (and tools\movie.py) and nothing else"
            }
        }
    }
}

$zip = Join-Path $out "$name.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
Remove-Item -Recurse -Force $stage
Show-Info "built on $builtOn, plugin API $pluginApi"
Show-Ok "package -> $zip"
