using System.ComponentModel;
using System.Runtime.CompilerServices;
using FuryEqualize.UI.Services;

namespace FuryEqualize.UI.ViewModels;

public class ChannelViewModel : INotifyPropertyChanged
{
    public int Index { get; }
    public string Name { get; }
    public string ShortName { get; }
    private float _gainLinear = 1.0f; // 0..2 (0..+6dB range roughly)
    public float GainLinear { get => _gainLinear; set { _gainLinear = Math.Clamp(value, 0f, 2f); OnPropertyChanged(); OnPropertyChanged(nameof(GainDb)); OnPropertyChanged(nameof(GainPercent)); if(AudioEngineInterop.IsAvailable) AudioEngineInterop.AudioEngine_SetChannelGain(Index, value); } }
    public double GainDb => 20*Math.Log10(Math.Max(GainLinear, 0.0001));
    public int GainPercent => (int)(GainLinear*100);
    public ChannelViewModel(int idx, string name, string shortName){ Index=idx; Name=name; ShortName=shortName; }
    public event PropertyChangedEventHandler? PropertyChanged;
    void OnPropertyChanged([CallerMemberName] string? n=null) => PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(n));
}