using System.Windows;

namespace FuryEqualize.UI;

public partial class App : System.Windows.Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);
        // Checagem Loudness Equalization no primeiro boot
        _ = Services.SystemCheckService.CheckLoudnessEqualizationAsync();
    }
}
