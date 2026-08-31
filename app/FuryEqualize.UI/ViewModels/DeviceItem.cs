namespace FuryEqualize.UI.ViewModels;

public class DeviceItem
{
    public string Id { get; set; } = "";
    public string Name { get; set; } = "";
    public int Channels { get; set; }
    public int SampleRate { get; set; }
    public bool IsDefault { get; set; }
    public override string ToString() => Name;
}
