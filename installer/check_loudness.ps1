# Verifica Loudness Equalization (best-effort) — executado no primeiro boot do app
# Conflita com a engine FuryEqualize se ativo.

$paths = @(
  "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render",
  "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture"
)

Write-Host "== FuryEqualize — Checagem Loudness Equalization ==" -ForegroundColor Cyan
Write-Host "Dica: Painel de Controle > Som > Propriedades do dispositivo > Enhancements > desmarque 'Loudness Equalization'" -ForegroundColor Yellow
Write-Host ""

# Varredura simplificada: procura por APOs que possam indicar EQ ativo
# (drivers diferentes expõem chaves diferentes; esta é heurística)
$found = $false
foreach ($base in $paths) {
  if (Test-Path $base) {
    Get-ChildItem $base -Recurse -ErrorAction SilentlyContinue | ForEach-Object {
      $props = Get-ItemProperty -Path $_.PsPath -ErrorAction SilentlyContinue
      if ($props -match "Loudness") { $found = $true; Write-Host "Encontrado: $($_.PsPath)" -ForegroundColor Red }
    }
  }
}
if (-not $found) { Write-Host "Nenhum indício de Loudness Equalization encontrado (heurística). Verifique manualmente nas propriedades do dispositivo." -ForegroundColor Green }
