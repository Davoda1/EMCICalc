# Source-development helper only; portable releases start with EMCICalc.exe.
[CmdletBinding()]
param([ValidateSet('help','build','test','package','clean')][string]$Action='help',
      [string]$BuildDir='', [string]$Prefix='')
$ErrorActionPreference='Stop'
$Root=$PSScriptRoot
if (!$BuildDir) { $BuildDir=Join-Path $Root 'build' }
if (!$Prefix) { $Prefix=$env:EMCICALC_PREFIX }
function Invoke-Checked([string]$Program,[string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
function Build-App {
    $Configure=@('-S',$Root,'-B',$BuildDir,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release')
    if (!$Prefix) { throw 'Provide -Prefix with one coherent MSYS2 MINGW64 compiler and dependency installation.' }
    $Prefix=[IO.Path]::GetFullPath($Prefix)
    $Compiler=Join-Path $Prefix 'bin/g++.exe'
    if (!(Test-Path -LiteralPath $Compiler)) { throw "Matching compiler is missing: $Compiler" }
    $Cache=Join-Path $BuildDir 'CMakeCache.txt'
    if (Test-Path -LiteralPath $Cache) {
        $CachedLine=Get-Content -LiteralPath $Cache | Where-Object { $_ -match '^CMAKE_CXX_COMPILER:[^=]+=' } | Select-Object -First 1
        if ($CachedLine) {
            $CachedCompiler=($CachedLine -split '=',2)[1]
            if ([IO.Path]::GetFullPath($CachedCompiler).Replace('\','/') -ne $Compiler.Replace('\','/')) {
                throw 'The build directory caches a different compiler. Use a fresh -BuildDir or clean this source build before changing compiler prefixes.'
            }
        }
    }
    $Configure+=('-DCMAKE_PREFIX_PATH='+$Prefix)
    $Configure+=('-DCMAKE_CXX_COMPILER='+$Compiler)
    $env:PATH=(Join-Path $Prefix 'bin')+';'+$env:PATH
    $env:PKG_CONFIG_PATH=(Join-Path $Prefix 'lib/pkgconfig')
    Invoke-Checked 'cmake' $Configure
    Invoke-Checked 'cmake' @('--build',$BuildDir,'--parallel','2')
}
switch ($Action) {
    'help' {
        Write-Output @'
EMCICalc Desktop Edition 1.0.0 development helper
  .\run.ps1 build -Prefix C:\msys64\mingw64
  .\run.ps1 test -Prefix C:\msys64\mingw64
  .\run.ps1 package -Prefix C:\msys64\mingw64
  .\run.ps1 clean
Use Qt 6 and libqalculate built for the same MinGW64 compiler and prefix.
The helper selects Prefix\bin\g++.exe and rejects a cached different compiler.
Requires CMake, Ninja, GCC C++17, pkg-config and Python 3 (for packaging).
Only development scripts are included with source; runtime ZIP has a native EXE.
'@
    }
    'build' { Build-App }
    'test' { Build-App; Invoke-Checked 'ctest' @('--test-dir',$BuildDir,'--output-on-failure') }
    'package' {
        if (!$Prefix) { throw 'Provide -Prefix with the MinGW64 dependency installation.' }
        Build-App
        Invoke-Checked 'python' @((Join-Path $Root 'scripts/package.py'),'--platform','windows','--compiled-definitions','--build-dir',$BuildDir,'--sysroot',$Prefix,'--output',(Join-Path $Root 'dist'))
    }
    'clean' {
        $Full=[IO.Path]::GetFullPath($BuildDir)
        if ($Full -eq [IO.Path]::GetFullPath($Root) -or $Full -eq [IO.Path]::GetPathRoot($Full)) { throw 'Refusing unsafe build directory.' }
        if (!(Test-Path -LiteralPath $Full)) { Write-Output 'Build directory does not exist.'; break }
        $Cache=Join-Path $Full 'CMakeCache.txt'
        $Expected='CMAKE_HOME_DIRECTORY:INTERNAL='+$Root.Replace('\','/')
        if (!(Test-Path -LiteralPath $Cache) -or !(Get-Content -LiteralPath $Cache | Where-Object { $_ -eq $Expected })) {
            throw 'Refusing to remove a directory that is not a configured build of this source tree.'
        }
        Invoke-Checked 'cmake' @('-E','rm','-rf',$Full)
    }
}
