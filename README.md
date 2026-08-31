# FuryEqualize — Rara Audio Clone

Desktop app estilo Rara Audio para equalização/compressão em tempo real.

## Arquitetura (3 camadas)
- **Core Engine (C++20 DLL)**: WASAPI Loopback (2ch estéreo / 8ch 7.1 via `WAVEFORMATEXTENSIBLE`), buffer 128/256/512 samples (~2.9–11.6ms @44.1kHz), DSP Biquad + Compressor `Quiet Gunshots`.
- **UI (C# WPF .NET 8)**: System Tray, FFI via P/Invoke, seletor de device/buffer, presets, hotkeys globais.
- **Backend (placeholder)**: API REST + Discord OAuth2 + distribuição segura de presets (sem salvar em texto plano).

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

## Roadmap
- [Mês 1] Core Engine WASAPI + DSP
- [Mês 2] UI WPF + Integração DLL
- [Mês 3] Backend API + OAuth Discord
- [Mês 4] Installer + Testes de latência
