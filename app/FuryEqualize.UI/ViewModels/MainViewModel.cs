using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Input;
using System.Windows.Threading;
using FuryEqualize.UI.Services;

namespace FuryEqualize.UI.ViewModels;

public class MainViewModel : INotifyPropertyChanged
{
    private readonly DispatcherTimer _meterTimer;
    public ObservableCollection<DeviceItem> Devices { get; } = new();
    public ObservableCollection<Preset> Presets { get; } = new(PresetService.Presets);
    public ObservableCollection<int> BufferSizes { get; } = new() { 128, 256, 512, 1024 };

    private DeviceItem? _selectedDevice;
    public DeviceItem? SelectedDevice { get => _selectedDevice; set { _selectedDevice = value; OnPropertyChanged(); } }

    // Render virtual cable (VB-Cable): auto-detect por padrão
    public ObservableCollection<DeviceItem> RenderDevices { get; } = new();
    private DeviceItem? _selectedRenderDevice;
    public DeviceItem? SelectedRenderDevice {
        get => _selectedRenderDevice;
        set { _selectedRenderDevice = value; OnPropertyChanged(); OnPropertyChanged(nameof(RenderDeviceLabel));
              if (AudioEngineInterop.IsAvailable && value != null) {
                  var id = value.Id == "__auto" ? null : value.Id == "__none" ? "none" : value.Id;
                  AudioEngineInterop.AudioEngine_SetRenderDevice(id);
              }
        }
    }
    public string RenderDeviceLabel => SelectedRenderDevice?.Name ?? "Auto (VB-Cable)";

    private Preset _selectedPreset = PresetService.Presets[0];
    public Preset SelectedPreset { get => _selectedPreset; set {
        _selectedPreset = value; OnPropertyChanged(); PresetService.Apply(value);
        // Se for Sniper, sincroniza master
        if(value.Name.Contains("Sniper")){
            try{ var sp = SniperPresetService.LoadDefault(); MasterVolumeDb = sp.masterVolumeDb; } catch{}
        }
        StatusText = $"Preset: {value.Name}";
        // Load channel gains from preset when selected
        LoadChannelGainsFromPreset(value);
    } }

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

    private float _masterVolumeDb = 6.0f;
    public float MasterVolumeDb { get => _masterVolumeDb; set { _masterVolumeDb = Math.Clamp(value, -24, 24); OnPropertyChanged(); OnPropertyChanged(nameof(MasterVolumePercent)); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetMasterVolume(_masterVolumeDb); } }
    public int MasterVolumePercent => (int)(Math.Pow(10, MasterVolumeDb/20)*100);

    // Sniper preset adjustable parameters (real-time control)
    private float _thresholdDb = -41f;
    public float ThresholdDb { get => _thresholdDb; set { _thresholdDb = Math.Clamp(value, -60, 0); OnPropertyChanged(); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetPreset(_thresholdDb, Ratio, AttackMs, ReleaseMs, MakeupDb); } }

    private float _ratio = 8f;
    public float Ratio { get => _ratio; set { _ratio = Math.Clamp(value, 1f, 20f); OnPropertyChanged(); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetPreset(ThresholdDb, _ratio, AttackMs, ReleaseMs, MakeupDb); } }

    private float _attackMs = 0.51f;
    public float AttackMs { get => _attackMs; set { _attackMs = Math.Clamp(value, 0.1f, 100f); OnPropertyChanged(); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetPreset(ThresholdDb, Ratio, _attackMs, ReleaseMs, MakeupDb); } }

    private float _releaseMs = 120f;
    public float ReleaseMs { get => _releaseMs; set { _releaseMs = Math.Clamp(value, 10f, 500f); OnPropertyChanged(); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetPreset(ThresholdDb, Ratio, AttackMs, _releaseMs, MakeupDb); } }

    private float _makeupDb = 5.98f;
    public float MakeupDb { get => _makeupDb; set { _makeupDb = Math.Clamp(value, -12, 24); OnPropertyChanged(); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetPreset(ThresholdDb, Ratio, AttackMs, ReleaseMs, _makeupDb); } }

    private float _lfeGate = -50f;
    public float LfeGate { get => _lfeGate; set { _lfeGate = Math.Clamp(value, -80, 0); OnPropertyChanged(); } }

    private float _frontLockEng = 4f;
    public float FrontLockEng { get => _frontLockEng; set { _frontLockEng = Math.Clamp(value, 0f, 20f); OnPropertyChanged(); } }

    private float _frontLockRel = 2f;
    public float FrontLockRel { get => _frontLockRel; set { _frontLockRel = Math.Clamp(value, 0f, 20f); OnPropertyChanged(); } }

    private float _fcCrack = -9.44f;
    public float FcCrack { get => _fcCrack; set { _fcCrack = value; OnPropertyChanged(); } }

    private float _tier2Centered = -3.6f;
    public float Tier2Centered { get => _tier2Centered; set { _tier2Centered = value; OnPropertyChanged(); } }

    // 7.1 Mixer (Sauda)
    public ObservableCollection<ChannelViewModel> Channels71 { get; } = new();
    private float _lfeGainDb = 0f;
    public float LfeGainDb { get => _lfeGainDb; set { _lfeGainDb = Math.Clamp(value, -24, 12); OnPropertyChanged(); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetChannelGain(3, (float)Math.Pow(10, value/20)); } }

    // Per-preset channel gain tracking (index -> gain linear)
    private readonly Dictionary<int, float[]> _presetChannelGains = new();
    private float[] _currentChannelGains;

    public ICommand ResetChannels71Command { get; }

    public MainViewModel()
    {
        ResetChannels71Command = new RelayCommand(() => {
            foreach(var c in Channels71) c.GainLinear = 1.0f;
            LfeGainDb = 0;
            StatusText = "7.1 Mixer resetado";
        });
        // 7.1 Mixer init (Sauda order: FL, FR, FC, LFE, BL, BR, SL, SR)
        var chNames = new[] { ("FL","FL"), ("FR","FR"), ("FC","FC"), ("LFE","LFE"), ("BL","BL"), ("BR","BR"), ("SL","SL"), ("SR","SR") };
        for(int i=0;i<8;i++) Channels71.Add(new ChannelViewModel(i, chNames[i].Item1, chNames[i].Item2));
        
        // Initialize current channel gains (8 channels)
        _currentChannelGains = new float[8];
        for(int i=0;i<8;i++) _currentChannelGains[i] = 1.0f;
        
        // Initialize preset channel gains from defaults
        foreach(var p in Presets){
            if(p.ChannelGains != null && p.ChannelGains.Length == 8){
                _presetChannelGains[Presets.IndexOf(p)] = (float[])p.ChannelGains.Clone();
            } else {
                _presetChannelGains[Presets.IndexOf(p)] = new float[8] { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
            }
        }
        
        RefreshDevices();
        // Load channel gains from initially selected preset
        LoadChannelGainsFromPreset(SelectedPreset);
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

    private void LoadChannelGainsFromPreset(Preset preset){
        var idx = Presets.IndexOf(preset);
        if(_presetChannelGains.TryGetValue(idx, out var gains)){
            _currentChannelGains = gains;
            for(int i=0;i<8;i++){
                Channels71[i].GainLinear = gains[i];
            }
        }
    }

    public void SaveChannelGainsToPreset(Preset preset){
        var idx = Presets.IndexOf(preset);
        _presetChannelGains[idx] = (float[])_currentChannelGains.Clone();
        // Note: Preset is immutable, gains tracked separately in _presetChannelGains dict
    }

    static DeviceItem Map(FuryDeviceInfo f) => new DeviceItem {
        Id = f.id ?? "", Name = string.IsNullOrWhiteSpace(f.name) ? (f.id ?? "") : f.name,
        Channels = f.channels, SampleRate = f.sampleRate, IsDefault = f.isDefault==1
    };

    public void RefreshDevices()
    {
        Devices.Clear();
        RenderDevices.Clear();
        if (!AudioEngineInterop.IsAvailable)
        {
            Devices.Add(new DeviceItem { Id = "", Name = "⚠ DLL não encontrada — modo mock", Channels = 2, SampleRate = 48000, IsDefault = true });
            SelectedDevice = Devices[0];
            RenderDevices.Add(new DeviceItem { Id = "__auto", Name = "Auto (detecta VB-Cable)", Channels = 2, SampleRate = 48000, IsDefault = true });
            RenderDevices.Add(new DeviceItem { Id = "__none", Name = "Sem render (só monitor)", Channels = 2, SampleRate = 48000, IsDefault = false });
            SelectedRenderDevice = RenderDevices[0];
            StatusText = "Core DLL não encontrada em output. Compile core/build/Release/FuryEqualizeCore.dll — modo local, sem auth";
            return;
        }
        try
        {
            int count = 0;
            try { count = AudioEngineInterop.AudioEngine_GetDevices(null!, 0); }
            catch (Exception ex) { StatusText = $"Falha ao enumerar devices: {ex.Message}"; count = 0; }

            if (count == 0)
            {
                StatusText = "Nenhum device WASAPI encontrado — usando fallback mock";
                Devices.Add(new DeviceItem { Id = "", Name = "Default (WASAPI Shared) — fallback", Channels = 2, SampleRate = 48000, IsDefault = true });
                SelectedDevice = Devices[0];
                RenderDevices.Add(new DeviceItem { Id = "__auto", Name = "Auto (detecta VB-Cable / Hi-Fi Cable)", Channels = 2, SampleRate = 48000, IsDefault = true });
                RenderDevices.Add(new DeviceItem { Id = "__none", Name = "Sem render (so monitor)", Channels = 2, SampleRate = 48000, IsDefault = false });
                SelectedRenderDevice = RenderDevices[0];
                try { SelectedBuffer = AudioEngineInterop.AudioEngine_GetBuffer(); } catch { SelectedBuffer = 256; }
                return;
            }

            var arr = new FuryDeviceInfo[Math.Min(count, 32)];
            AudioEngineInterop.AudioEngine_GetDevices(arr, arr.Length);
            foreach (var f in arr) Devices.Add(Map(f));
            var def = Devices.FirstOrDefault(d => d.IsDefault);
            SelectedDevice = def ?? Devices[0];

            RenderDevices.Add(new DeviceItem { Id = "__auto", Name = "Auto (detecta VB-Cable / Hi-Fi Cable)", Channels = 2, SampleRate = 48000, IsDefault = true });
            foreach (var f in arr) RenderDevices.Add(Map(f));
            RenderDevices.Add(new DeviceItem { Id = "__none", Name = "Sem render (so monitor)", Channels = 2, SampleRate = 48000, IsDefault = false });
            SelectedRenderDevice = RenderDevices[0];
            AudioEngineInterop.AudioEngine_SetRenderDevice(null);

            SelectedBuffer = AudioEngineInterop.AudioEngine_GetBuffer();
            StatusText = $"{count} device(s) encontrado(s) — modo local, sem auth";
        }
        catch (Exception ex)
        {
            Devices.Add(new DeviceItem { Id = "", Name = $"Erro: {ex.Message}", Channels = 2, SampleRate = 48000, IsDefault = true });
            SelectedDevice = Devices[0];
            RenderDevices.Add(new DeviceItem { Id = "__auto", Name = "Auto (detecta VB-Cable)", Channels = 2, SampleRate = 48000, IsDefault = true });
            SelectedRenderDevice = RenderDevices[0];
            StatusText = $"Erro ao listar devices: {ex.Message}";
        }
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
            var id = SelectedDevice?.Id;
            var rc = AudioEngineInterop.AudioEngine_Start(string.IsNullOrEmpty(id) ? null : id);
            IsRunning = rc == 0;
            StatusText = rc == 0 ? $"Engine rodando @ {SelectedBuffer} samples (~{SelectedBuffer/48.0:F1}ms @48kHz)" : $"Falha ao iniciar: {rc}";
            if (IsRunning) PresetService.Apply(SelectedPreset);
        }
    }

    public event PropertyChangedEventHandler? PropertyChanged;
    void OnPropertyChanged([CallerMemberName] string? n=null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));
}
