using System.Windows;
using System.Windows.Interop;
using FuryEqualize.UI.Helpers;
using FuryEqualize.UI.Services;
using FuryEqualize.UI.ViewModels;
using Forms = System.Windows.Forms;

namespace FuryEqualize.UI;

public partial class MainWindow : Window
{
    private readonly MainViewModel _vm = new();
    private HotkeyManager? _hotkeys;
    private Forms.NotifyIcon? _tray;

    public MainWindow()
    {
        InitializeComponent();
        DataContext = _vm;
        Loaded += OnLoaded;
        Closing += OnClosing;
        StateChanged += OnStateChanged;
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        var hwnd = new WindowInteropHelper(this).Handle;
        if (hwnd != IntPtr.Zero) SetupHotkeys(hwnd);

        // System Tray
        _tray = new Forms.NotifyIcon
        {
            Text = "FuryEqualize",
            Visible = true,
            Icon = LoadAppIcon()
        };
        var menu = new Forms.ContextMenuStrip();
        menu.Items.Add("Abrir", null, (_, _) => { Show(); WindowState = WindowState.Normal; Activate(); });
        menu.Items.Add("Iniciar/Parar Engine", null, (_, _) => _vm.ToggleEngine());
        menu.Items.Add(new Forms.ToolStripSeparator());
        foreach (var p in PresetService.Presets)
        {
            var preset = p;
            menu.Items.Add(preset.Name, null, (_, _) => _vm.SelectedPreset = preset);
        }
        menu.Items.Add(new Forms.ToolStripSeparator());
        menu.Items.Add("Sair", null, (_, _) => { _tray.Visible = false; System.Windows.Application.Current.Shutdown(); });
        _tray.ContextMenuStrip = menu;
        _tray.DoubleClick += (_, _) => { Show(); WindowState = WindowState.Normal; };
    }

    private static System.Drawing.Icon LoadAppIcon()
    {
        try
        {
            var sri = System.Windows.Application.GetResourceStream(
                new Uri("pack://application:,,,/Assets/App.ico"));
            if (sri != null)
            {
                using (sri.Stream)
                {
                    return new System.Drawing.Icon(sri.Stream);
                }
            }
        }
        catch { }
        return System.Drawing.SystemIcons.Application;
    }

    private void SetupHotkeys(IntPtr hwnd)
    {
        _hotkeys = new HotkeyManager(hwnd);
        // Ctrl+Alt+F1/F2/F3 -> presets
        _hotkeys.Register(1, 0x70, true, true, false, () => Dispatcher.Invoke(() => _vm.SelectedPreset = PresetService.Presets[0])); // F1
        _hotkeys.Register(2, 0x71, true, true, false, () => Dispatcher.Invoke(() => _vm.SelectedPreset = PresetService.Presets[1])); // F2
        _hotkeys.Register(3, 0x72, true, true, false, () => Dispatcher.Invoke(() => _vm.SelectedPreset = PresetService.Presets[2])); // F3
    }

    private void OnToggleEngine(object sender, RoutedEventArgs e) => _vm.ToggleEngine();
    private void OnRefresh(object sender, RoutedEventArgs e) => _vm.RefreshDevices();
    private void OnMinimizeClick(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void OnMaximizeRestoreClick(object sender, RoutedEventArgs e) =>
        WindowState = WindowState == WindowState.Maximized ? WindowState.Normal : WindowState.Maximized;
    private void OnCloseButtonClick(object sender, RoutedEventArgs e) => Close();

    private void OnStateChanged(object? sender, EventArgs e)
    {
        if (WindowState == WindowState.Minimized) Hide(); // Eco Mode: minimiza para tray
    }

    private void OnClosing(object? sender, System.ComponentModel.CancelEventArgs e)
    {
        // Fecha para tray em vez de encerrar (se engine rodando)
        if (_vm.IsRunning)
        {
            e.Cancel = true;
            Hide();
            _tray?.ShowBalloonTip(1500, "FuryEqualize", "Rodando em segundo plano (Eco Mode). Clique no tray para restaurar.", Forms.ToolTipIcon.Info);
        }
        else
        {
            _hotkeys?.Dispose();
            if (_tray != null) { _tray.Visible = false; _tray.Dispose(); }
        }
    }
}
