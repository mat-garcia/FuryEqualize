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

## Backend
- Ver `backend/openapi.yaml`. Presets nunca em texto plano no disco; coeficientes vêm assinados (HMAC) e são injetados direto na DLL em memória.

## Próximos passos técnicos
- Fase 1.5: adicionar `IAudioRenderClient` para VB-Cable (hoje só captura/monitora).
- Resampling se mix != 48kHz.
- Assinatura HMAC no client antes de aplicar preset remoto.
