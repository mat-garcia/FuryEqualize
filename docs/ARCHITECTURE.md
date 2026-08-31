# Arquitetura FuryEqualize

## Fluxo de áudio
```
Jogo -> Windows Mix (WASAPI Shared) -> Loopback Capture (FuryEqualizeCore.dll) 
  -> Biquad x3 (LowShelf 150Hz, Peak 3.5kHz Q1.2, HighShelf 10kHz) 
  -> Compressor (Envelope Follower, thr/ratio/attack/release/makeup + soft knee)
  -> Render para VB-Cable / Hi-Fi Cable (device virtual) -> Fone / Saída física
```
- Buffer: 128/256/512/1024 samples. Alvo 256 (~5.3ms @48kHz) + 1.5x margem WASAPI.
- Formato: `WAVEFORMATEXTENSIBLE`, 2ch (estéreo) e 8ch (7.1) — 7.1 processado por pares.

## UI (WPF)
- `MainWindow.xaml` lista devices via `AudioEngine_GetDevices` (MMDeviceEnumerator).
- `AudioEngineInterop.cs` faz P/Invoke da DLL (CallingConvention Cdecl).
- System Tray via `NotifyIcon` (WinForms), Eco Mode (hide on minimize/close while running).
- Hotkeys globais `Ctrl+Alt+F1/F2/F3` via `RegisterHotKey` (user32).
- Telemetria: polling 80ms de `GetInputLevelDb` / `GetGainReductionDb`.

## Modo local (sem backend)
- Presets locais em `Services/PresetService.cs:9` — sem JWT/OAuth. `backend/openapi.yaml` e `LicenseService.cs` mantidos só como referência (Obsolete).
- `Services/PresetCrypto.cs` desabilitado em modo local.

## Estado atual (Fase 1.5 concluída)
- `wasapi_engine.cpp:150` já tem dual-client `IAudioCaptureClient` + `IAudioRenderClient` para VB-Cable (auto-detect), conversão PCM16/float e 7.1 multicanal.

## Próximos passos
- Resampler quando captura e render têm sampleRate diferentes (placeholder atual apenas loga drift).
- Testes de latência in-game e instalador com VB-Cable empacotado.
