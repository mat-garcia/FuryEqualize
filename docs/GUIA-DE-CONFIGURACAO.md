# FuryEqualize — Guia de Configuração e Uso

Documento de apoio pra colocar o app rodando do zero, explicar o fluxo local e resolver os problemas comuns (incluindo os selects "brancos").

---

## 1. O que precisa antes de rodar

| Item | Onde pegar |
|------|------------|
| Windows 10/11 x64 | já deve ter |
| .NET 8 SDK (ou superior) | `winget install Microsoft.DotNet.SDK.8` ou https://dotnet.microsoft.com/download |
| Visual Studio 2022 (C++ Desktop) | VS Installer |
| CMake >= 3.20 | `winget install Kitware.CMake` |
| VB-Cable | https://vb-audio.com/Cable/ (gratuito) — **criar o "CABLE Input"** |

> O app rola 100% local: sem backend, sem login, sem chave. `backend/` é só referência.

---

## 2. Passo a passo pra funcionar

### 2.1 Instalar o VB-Cable
1. Baixe e instale https://vb-audio.com/Cable/
2. Reinicie se o dispositivo "CABLE Input" / "CABLE Output" não aparecer.
3. No Windows, desligue o **Loudness Equalization** da saída de áudio (Painel de Controle → Som → dispositivo de saída → Avançado → Retire o check). O JS do instalador (Inno Setup) já tenta fazer isso.

### 2.2 Build da engine (C++ → DLL)
```bat
cmake -S core -B core/build -A x64
cmake --build core/build --config Release
```
Gera `core/build/Release/FuryEqualizeCore.dll`.

### 2.3 Build da interface (WPF)
```bat
dotnet build app/FuryEqualize.UI/FuryEqualize.UI.csproj -c Release
```

### 2.4 Rodar
```bat
dotnet run --project app/FuryEqualize.UI/FuryEqualize.UI.csproj
```
> A DLL da engine precisa estar acessível: ou copie `core/build/Release/FuryEqualizeCore.dll` pra pasta de saída da UI, ou use a versão self-contained (`FuryEqualize-v0.2-Rara-SELFCONTAINED/`), que já vem com a DLL.

### 2.5 Usar (ordem dos selects)
1. **Captura (loopback)** → selecione a sua saída de áudio (ex: fone/auto-falante por onde sai o jogo).
2. **Render (fone / VB-Cable)** → selecione **CABLE Input** (ou deixe auto-detectado / `none` = só monitora sem reemitir).
3. **Buffer (latência)** → 256 (padrão ~5.3ms@48kHz) ou menor se o PC aguentar.
4. **PRESET** → escolha `Competitive` / `Quiet Gunshots` / `Flat` (ou ajuste na seção "Parâmetros Reais").
5. Clique **Iniciar**.

### 2.6 Gerar o .exe único (portátil, sem instalar nada)
O `FuryEqualize.UI.csproj` já vem configurado (Release) para publicar tudo num só arquivo:
- `SelfContained` + .NET 8 embutido → **não precisa instalar .NET**;
- `IncludeNativeLibrariesForSelfExtract` + `IncludeAllContentForSelfExtract` → o `FuryEqualizeCore.dll` (engine C++) e os presets vão DENTRO do exe e são extraídos pro temp na primeira execução;
- `ApplicationIcon` → `Assets/App.ico` (ícone no arquivo, na janela e no tray);

Comando:
```bat
dotnet publish app/FuryEqualize.UI/FuryEqualize.UI.csproj -c Release -o publish/dist
```
Resultado: **`publish/dist/FuryEqualize.exe`** (~155 MB). Copie só esse arquivo pra qualquer máquina Windows x64 → duplo clique → pronto.

> Se quiser o exe menor, remova `IncludeAllContentForSelfExtract` do csproj — aí a DLL e os presets ficam fora do exe (na mesma pasta).

---

## 3. Como o áudio flui
```
Jogo -> Windows Mix (WASAPI Shared) -> Loopback Capture (FuryEqualizeCore.dll)
  -> EQ Biquad (LowShelf 150Hz, Peak 3.5kHz, HighShelf 10kHz)
  -> Compressor (Threshold/Ratio/Attack/Release/Makeup + soft knee)
  -> Render -> VB-Cable -> saída física (fone)
```
- Dual-client: captura do seu device + render pro VB-Cable ao mesmo tempo.
- Bancos 2ch (estéreo) e 8ch (7.1, processado por pares).
- Conversão PCM16 ↔ float feita na DLL.

---

## 4. Problemas comuns

### 4.1 "Os selects ficam brancos ao selecionar" ✅ resolvido
**Causa:** o template padrão do ComboBox da WPF traz o dropdown com fundo branco/tema Aero e fonte pequena; no tema escuro do app destoava.

**Correção aplicada em `app/FuryEqualize.UI/MainWindow.xaml`:**
- Novo `Style TargetType="ComboBox"` com template escuro próprio (`#121A23`), borda arredondada, seta customizada, fonte `Segoe UI` 12.
- Novo `Style TargetType="ComboBoxItem"` escuro: item normal `#D7E0E8`, hover `#22303F`, selecionado com accent `#22D3EE`.

Pra recompilar a UI com a correção:
```bat
dotnet build app/FuryEqualize.UI/FuryEqualize.UI.csproj -c Release
```

### 4.2 Não acha o VB-Cable
- Confirme que está instalado e que o device aparece em Configurações → Som.
- Use o botão **↻ Atualizar** na tela antes de selecionar.

### 4.3 Sem som depois de Iniciar
- Verifique no Windows a saída de escuta: o app reemite no "CABLE Input"; o que escuta tem que ser o "CABLE Output".
- Confirme que a opção "Escutar este dispositivo" não está gerando loop de volume.

### 4.4 Latência / crackle
- Suba o buffer (512/1024) em 7.1 ou em máquinas mais fracas.
- Feche outros apps que usem WASAPI exclusivo.

### 4.5 Precisa do backend?
Não. Em modo local não existe servidor. Qualquer referência a JWT/OAuth (`backend/openapi.yaml`, `LicenseService`) é resíduo de uma versão anterior e está desativada.

---

## 5. Estrutura útil
```
core/        engine C++ (CMake + MSVC) → FuryEqualizeCore.dll
app/         FuryEqualize.UI (WPF .NET 8)
docs/        arquitetura + este guia
installer/   Inno Setup + checagem Loudness Equalization
presets/     presets em JSON usados pelo PresetService
```