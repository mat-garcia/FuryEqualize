using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Threading;
using FuryEqualize.UI.Services;

namespace FuryEqualize.UI.ViewModels;

public class MainViewModel : INotifyPropertyChanged
{
    private readonly DispatcherTimer _meterTimer;
    public ObservableCollection<FuryDeviceInfo> Devices { get; } = new();
    public ObservableCollection<Preset> Presets { get; } = new(PresetService.Presets);
    public ObservableCollection<int> BufferSizes { get; } = new() { 128, 256, 512, 1024 };

    private FuryDeviceInfo? _selectedDevice;
    public FuryDeviceInfo? SelectedDevice { get => _selectedDevice; set { _selectedDevice = value; OnPropertyChanged(); } }

    // Render virtual cable (VB-Cable): auto-detect por padrão
    public ObservableCollection<FuryDeviceInfo> RenderDevices { get; } = new();
    private FuryDeviceInfo? _selectedRenderDevice;
    public FuryDeviceInfo? SelectedRenderDevice {
        get => _selectedRenderDevice;
        set { _selectedRenderDevice = value; OnPropertyChanged(); OnPropertyChanged(nameof(RenderDeviceLabel));
              if (AudioEngineInterop.IsAvailable && value != null) {
                  // "auto" = deixa engine auto-detectar; senão usa id explícito
                  var id = value.Value.id == "__auto" ? null : value.Value.id == "__none" ? "none" : value.Value.id;
                  AudioEngineInterop.AudioEngine_SetRenderDevice(id);
              }
        }
    }
    public string RenderDeviceLabel => SelectedRenderDevice?.name ?? "Auto (VB-Cable)";

    private Preset _selectedPreset = PresetService.Presets[0];
    public Preset SelectedPreset { get => _selectedPreset; set { _selectedPreset = value; OnPropertyChanged(); PresetService.Apply(value); StatusText = $"Preset: {value.Name}"; } }

    private int _selectedBuffer = 256;
    public int SelectedBuffer { get => _selectedBuffer; set { _selectedBuffer = value; OnPropertyChanged(); if (AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetBuffer(value); } }

    private bool _isRunning;
    public bool IsRunning { get => _isRunning; set { _isRunning = value; OnPropertyChanged(); OnPropertyChanged(nameof(StartStopLabel)); } }
    public string StartStopLabel => IsRunning ? "Parar Engine" : "Iniciar Engine";

    private string _statusText = "Pronto";
    public string StatusText { get => _statusText; set { _statusText = value; OnPropertyChanged(); } }

    private double _inputLevel = -60;
    public double InputLevel { get => _inputLevel; set { _inputLevel = value; OnPropertyChanged(); } }

    private double _gainReduction;
    public double GainReduction { get => _gainReduction; set { _gainReduction = value; OnPropertyChanged(); } }

    public MainViewModel()
    {
        RefreshDevices();
        _meterTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(80) };
        _meterTimer.Tick += (_, _) =>
        {
            if (IsRunning && AudioEngineInterop.IsAvailable)
            {
                InputLevel = AudioEngineInterop.AudioEngine_GetInputLevelDb();
                GainReduction = AudioEngineInterop.AudioEngine_GetGainReductionDb();
            }
        };
        _meterTimer.Start();
    }

    public void RefreshDevices()
    {
        Devices.Clear();
        RenderDevices.Clear();
        if (!AudioEngineInterop.IsAvailable)
        {
            Devices.Add(new FuryDeviceInfo { id = "", name = "⚠ DLL não encontrada — modo mock", channels = 2, sampleRate = 48000, isDefault = 1 });
            SelectedDevice = Devices[0];
            RenderDevices.Add(new FuryDeviceInfo { id = "__auto", name = "Auto (detecta VB-Cable)", channels = 2, sampleRate = 48000, isDefault = 1 });
            RenderDevices.Add(new FuryDeviceInfo { id = "__none", name = "Sem render (só monitor)", channels = 2, sampleRate = 48000, isDefault = 0 });
            SelectedRenderDevice = RenderDevices[0];
            StatusText = "Core DLL não encontrada em output. Compile core/build/Release/FuryEqualizeCore.dll — modo local, sem auth";
            return;
        }
        int count = AudioEngineInterop.AudioEngine_GetDevices(null!, 0);
        if (count == 0) { StatusText = "Nenhum device encontrado"; return; }
        var arr = new FuryDeviceInfo[Math.Min(count, 32)];
        AudioEngineInterop.AudioEngine_GetDevices(arr, arr.Length);
        foreach (var d in arr) Devices.Add(d);
        var def = Devices.FirstOrDefault(d => d.isDefault == 1);
        SelectedDevice = def.name != null ? def : Devices[0];

        // Render devices = mesmos devices + opções especiais
        RenderDevices.Add(new FuryDeviceInfo { id = "__auto", name = "Auto (detecta VB-Cable / Hi-Fi Cable)", channels = 2, sampleRate = 48000, isDefault = 1 });
        foreach (var d in arr) RenderDevices.Add(d);
        RenderDevices.Add(new FuryDeviceInfo { id = "__none", name = "Sem render (só monitor)", channels = 2, sampleRate = 48000, isDefault = 0 });
        SelectedRenderDevice = RenderDevices[0];
        // Aplica auto por padrão
        AudioEngineInterop.AudioEngine_SetRenderDevice(null);

        SelectedBuffer = AudioEngineInterop.AudioEngine_GetBuffer();
        StatusText = $"{count} device(s) encontrado(s) — modo local, sem auth";
    }

    public void ToggleEngine()
    {
        if (!AudioEngineInterop.IsAvailable) { StatusText = "DLL ausente — compile o Core primeiro"; return; }
        if (IsRunning)
        {
            var rc = AudioEngineInterop.AudioEngine_Stop();
            IsRunning = false;
            StatusText = rc == 0 ? "Engine parada" : $"Erro ao parar: {rc}";
        }
        else
        {
            var id = SelectedDevice?.id;
            var rc = AudioEngineInterop.AudioEngine_Start(string.IsNullOrEmpty(id) ? null : id);
            IsRunning = rc == 0;
            StatusText = rc == 0 ? $"Engine rodando @ {SelectedBuffer} samples (~{SelectedBuffer/48.0:F1}ms @48kHz)" : $"Falha ao iniciar: {rc}";
            if (IsRunning) PresetService.Apply(SelectedPreset);
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    void OnPropertyChanged([CallerMemberName] string? n=null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));
}
