param(
  [ValidateSet("Debug","Release")]$Config = "Release",
  [switch]$SkipCore,
  [switch]$SkipUI
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

Write-Host "== FuryEqualize build ($Config) ==" -ForegroundColor Cyan

if (-not $SkipCore) {
  Write-Host "`n[1/2] Core C++ (WASAPI + DSP)..." -ForegroundColor Yellow
  $cmake = Get-Command cmake -ErrorAction SilentlyContinue
  if (-not $cmake) {
    Write-Host "  cmake não encontrado — instale via: winget install Kitware.CMake" -ForegroundColor Red
    Write-Host "  OU abra core com Visual Studio 2022 (CMake integrado) e compile." -ForegroundColor Yellow
  } else {
    cmake -S "$root/core" -B "$root/core/build" -A x64
    cmake --build "$root/core/build" --config $Config
    $dll = "$root/core/build/$Config/FuryEqualizeCore.dll"
    if (Test-Path $dll) { Write-Host "  OK: $dll" -ForegroundColor Green }
    # Copia para output da UI para P/Invoke funcionar no run
    $dest = "$root/app/FuryEqualize.UI/bin/$Config/net8.0-windows/FuryEqualizeCore.dll"
    if (Test-Path $dll) { Copy-Item $dll $dest -Force; Write-Host "  Copiado para $dest" -ForegroundColor Green }
  }
}

if (-not $SkipUI) {
  Write-Host "`n[2/2] UI WPF..." -ForegroundColor Yellow
  dotnet build "$root/app/FuryEqualize.UI/FuryEqualize.UI.csproj" -c $Config
  Write-Host "`nRun: dotnet run --project $root/app/FuryEqualize.UI/FuryEqualize.UI.csproj -c $Config" -ForegroundColor Cyan
}

Write-Host "`nBuild concluído. Sem backend/auth — modo local." -ForegroundColor Green
