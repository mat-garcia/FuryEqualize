<p align="center">
  <img src="docs/furyeq.png" alt="Fury Equalize" width="180" height="180"/>
</p>

# 🎧 Fury Equalize

Desktop app estilo Rara Audio para equalização/compressão em tempo real.

**Fury Equalize serve pra tu ouvir até a respiração dos inimigos, totalmente free e sem risco de ban.**

- Usamos do VB-Cable para filtrar os canais de áudio do seu PC, tentando focar nos passos dos inimigos de forma legítima **sem mexer nos arquivos do jogo**.
- Simplesmente uma super equalização que serve pra qualquer jogo de FPS que queira usar.

## Como usar (versão final — sem instalar nada)

### 1. Pegar só o exe

Baixe o **FuryEqualize.exe** na release oficial (ZIP com o exe):

**[⬇️ Clique aqui](https://github.com/mat-garcia/FuryEqualize/releases/tag/V1.00-beta)**

Ele **já embute o .NET 8 e a engine C++** — não precisa instalar SDK, runtime ou dependência nenhuma.

### 2. Instalar o VB-Cable

Baixe e instale o VB-Cable: https://vb-audio.com/Cable/
É o "cabo virtual" que leva o som do jogo até o FuryEqualize. Se os devices `CABLE Input` / `CABLE Output` não aparecerem, reinicie o PC.

### 3. Setar o VB-Cable como som principal do PC

1. Abra **Configurações → Sistema → Som → Saída**.
2. Em "Escolher onde reproduzir o som", selecione **CABLE Input**.
3. Pronto: todo o som do Windows e do jogo passa a ir pro cabo virtual.

### 4. Abrir o FuryEqualize e configurar

1. **Captura (loopback)** → selecione **CABLE Input** — é o som do jogo que está passando pelo cabo.
2. **Render (saída)** → selecione **o fone do cara** (a saída física/headset) — é pra lá que o som processado vai tocar.
3. **Buffer (latência)** → `256` (~5ms) ou menos se o PC aguentar; `512/1024` em 7.1.
4. **PRESET** → `Competitive`, `Quiet Gunshots`, `Footsteps MAX` etc.
5. Clique **Iniciar** e jogue.

Fluxo final:

```
Jogo -> CABLE Input (VB-Cable) -> FuryEqualize processa -> Fone do cara
```

> Se o jogo estiver configurado pra outra saída de áudio, troque a saída dele pra **CABLE Input** também (senão o som não passa pelo cabo).
> Lembrando: com esse setup o som direto no fone só existe com o FuryEqualize rodando. Pra voltar ao normal, feche o app e redefina a saída padrão do Windows.

### 5. Atalhos

- `Ctrl+Alt+F1` / `F2` / `F3` — trocam de preset rapidinho.
- Minimizar → o app vai pro **tray** (e continua processando).

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

## C-API completa

- `AudioEngine_SetRenderDevice(const char*)` — `nullptr`=auto VB-Cable, `"none"`=só monitor
- `AudioEngine_SetPreset` / `SetEq` / `SetBuffer` / `GetDevices` etc. Ver `core/include/audio_engine.h:28`
