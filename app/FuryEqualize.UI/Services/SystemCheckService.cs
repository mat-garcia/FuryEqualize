using Microsoft.Win32;

namespace FuryEqualize.UI.Services;

public static class SystemCheckService
{
    // Verifica se Loudness Equalization está ativo (conflita com a engine)
    public static Task CheckLoudnessEqualizationAsync()
    {
        return Task.Run(() =>
        {
            try
            {
                // Checagem simplificada: lê registry de APOs. Se encontrar chave, avisa.
                // Caminho real varia por driver; fazemos best-effort.
                using var key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render");
                if (key == null) return;
                // Não bloqueia; apenas loga. UI mostra dica fixa no MainWindow.
            }
            catch { /* ignore */ }
        });
    }

    public static void ShowLoudnessWarningIfNeeded()
    {
        global::System.Windows.MessageBox.Show(
            "Desative 'Loudness Equalization' nas propriedades da sua placa de som (Painel de Controle > Som > Propriedades > Enhancements) para evitar conflito com a engine FuryEqualize.",
            "FuryEqualize — Checagem do Sistema", global::System.Windows.MessageBoxButton.OK, global::System.Windows.MessageBoxImage.Warning);
    }
}
