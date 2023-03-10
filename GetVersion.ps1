param (
    [String]$OutputFile = ".\Version.h",
    [String]$ProjectDirectory = "."
)

Push-Location $ProjectDirectory

$gitVersion = gitVersion.exe | ConvertFrom-Json
$semver = $gitVersion.semver
# $majorVersion = $gitVersion.Major
# $minorVersion = $gitVersion.Minor
# $patchVersion = $gitVersion.Patch
$shaShort = $gitVersion.ShortSha
$fullBuildMetaData = $gitVersion.FullBuildMetaData

$workingDir = Split-Path -Path $pwd -Leaf

Set-Content -Path $OutputFile "#define SemanticVersion ""$semver"""
# Add-Content -Path $OutputFile "#define MajorVersion ($majorVersion)"
# Add-Content -Path $OutputFile "#define MinorVersion ($minorVersion)"
# Add-Content -Path $OutputFile "#define PatchVersion ($patchVersion)"
Add-Content -Path $OutputFile "#define SHA_short ""$shaShort"""
Add-Content -Path $OutputFile "#define FullBuildMetaData ""$fullBuildMetaData"""
Add-Content -Path $OutputFile "#define WorkingDirectory ""$workingDir"""
Write-Debug "wrote semantic version: $semver"
Write-Host "-DSEMANTIC_VERSION=$semver"
Pop-Location