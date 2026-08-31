# FuryEqualize — Rara Audio Clone

Desktop app estilo Rara Audio para equalização/compressão em tempo real.

## Arquitetura (2 camadas — modo local, sem comercialização)
- **Core Engine (C++20 DLL)**: WASAPI Loopback + Render para VB-Cable (dual client), 2ch/7.1, buffer 128/256/512/1024 (~2.7–21ms @48kHz), DSP Biquad + Compressor `Quiet Gunshots`, conversão PCM16/float.
- **UI (C# WPF .NET 8)**: System Tray, FFI via P/Invoke, seletor captura/render/buffer, presets locais (`Competitive`/`Quiet Gunshots`/`Flat`), hotkeys globais `Ctrl+Alt+F1-F3`.
- **Backend**: removido — app roda 100% local. `backend/openapi.yaml` mantido só como referência.

## Estrutura
```
core/       -> C++ DLL (CMake + MSVC)
app/        -> FuryEqualize.UI (WPF)
backend/    -> contrato OpenAPI + mock
installer/  -> Inno Setup + checagem Loudness Equalization
docs/       -> arquitetura e guias
```

## Requisitos de Build
- Windows 10/11 x64
- Visual Studio 2022 + C++ Desktop + .NET 8 SDK
- CMake >= 3.20 (instalar via `winget install Kitware.CMake` ou VS Installer)
- .NET 8 SDK (`dotnet --version` deve retornar 8.x+ — 10.x também ok)

## Build Rápido

### Core (C++)
```bat
cmake -S core -B core/build -A x64
cmake --build core/build --config Release
:: gera core/build/Release/FuryEqualizeCore.dll
```

### UI (WPF)
```bat
dotnet build app/FuryEqualize.UI/FuryEqualize.UI.csproj -c Release
dotnet run --project app/FuryEqualize.UI/FuryEqualize.UI.csproj
```

### Installer
Abrir `installer/installer.iss` no Inno Setup 6 e compilar.

## C-API Exportada (core/include/audio_engine.h)
- `AudioEngine_Start(const char* deviceId)`
- `AudioEngine_Stop()`
- `AudioEngine_SetPreset(float threshold, float ratio, float attack, float release, float makeup)`
- `AudioEngine_SetBuffer(int bufferSize)`
- `AudioEngine_SetEq(float lowShelfGain, float peakGain, float highShelfGain)`
- `AudioEngine_GetDevices(...)` / `AudioEngine_IsRunning()`

## Uso rápido (sem auth)
1. Instale VB-Cable (https://vb-audio.com/Cable/) — será auto-detectado; ou selecione manualmente no dropdown Render.
2. `.\build.ps1 -Config Release` (ou `cmake -S core -B core/build -A x64 && cmake --build core/build --config Release` + `dotnet run --project app/FuryEqualize.UI`)
3. Na UI: selecione Captura (seu device) e Render (CABLE Input), escolha preset e clique Iniciar.

## C-API completa
- `AudioEngine_SetRenderDevice(const char*)` — `nullptr`=auto VB-Cable, `"none"`=só monitor
- `AudioEngine_SetPreset` / `SetEq` / `SetBuffer` / `GetDevices` etc. Ver `core/include/audio_engine.h:28`

## Roadmap (modo local)
- [x] Core WASAPI dual-client + DSP
- [x] UI WPF tray + hotkeys + render selector
- [ ] Resampler (quando captura e render têm sampleRate diferentes)
- [ ] Testes de latência in-game
