# Recreates the SDL3 and VRSFML working copies the Switch port is developed in.
#
# Those trees live outside the repository on purpose: what is committed here are the patches
# under patches/, which is the single source of truth. The working copies are scratch space
# -- roughly a gigabyte of them -- and nothing is lost by deleting them, as long as this
# script exists to bring them back.
#
# Each is cloned at the exact revision its patch was generated against (the same revisions
# the root CMakeLists pins), put on a switch-backend branch, and patched. Building the engine
# does NOT need them: the engine fetches and patches its own copies under
# build_switch_engine/_deps/. They are only needed to *change* the port, and the round trip
# back into a build is lab/switch_smoke/sync_switch_patches.ps1.
#
#     powershell -File lab/switch_smoke/setup_working_copies.ps1
#     powershell -File lab/switch_smoke/setup_working_copies.ps1 -Force   # replace existing

param(
    [string]$RepositoryRoot = "D:\deceptus\deceptus_engine",
    [string]$SdlWorkingCopy = "D:\deceptus\sdl3_switch",
    [string]$VrsfmlWorkingCopy = "D:\deceptus\vrsfml_switch",
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function New-WorkingCopy {
    param(
        [string]$Url,
        [string]$BaseRevision,
        [string]$Destination,
        [string]$PatchPath
    )

    if (Test-Path $Destination) {
        if (-not $Force) {
            Write-Output "$Destination already exists - pass -Force to replace it"
            return
        }

        Write-Output "removing $Destination"
        Remove-Item $Destination -Recurse -Force
    }

    # core.autocrlf=false for the clone: the patches are LF, and applying an LF patch to a
    # CRLF checkout is the trap this port keeps walking into. Keeping the whole tree LF also
    # means files mirrored out of here into the container stay LF.
    Write-Output "cloning $Url -> $Destination"
    git -c core.autocrlf=false clone --no-checkout $Url $Destination

    Push-Location $Destination
    try {
        git -c advice.detachedHead=false checkout -b switch-backend $BaseRevision
        git apply --whitespace=nowarn $PatchPath
        # staged, because sync_switch_patches.ps1 diffs --cached against the base revision and
        # that is the only way the files the patch adds show up in a regenerated patch
        git add -A
    } finally {
        Pop-Location
    }

    Write-Output "  applied $(Split-Path $PatchPath -Leaf)"
}

New-WorkingCopy -Url "https://github.com/vittorioromeo/SDL.git" `
                -BaseRevision "e205361fb67ff53868dbc333eb2c491e11ff1a51" `
                -Destination $SdlWorkingCopy `
                -PatchPath (Join-Path $RepositoryRoot "patches\switch-sdl3-backend.patch")

New-WorkingCopy -Url "https://github.com/vittorioromeo/VRSFML.git" `
                -BaseRevision "9c272d60134d568f35fbad9891f3b436de87cc28" `
                -Destination $VrsfmlWorkingCopy `
                -PatchPath (Join-Path $RepositoryRoot "patches\switch-vrsfml-backend.patch")

Write-Output "done"
