using System.Runtime.InteropServices;
using System.Windows.Interop;

namespace FuryEqualize.UI.Helpers;

public class HotkeyManager : IDisposable
{
    [DllImport("user32.dll")] static extern bool RegisterHotKey(IntPtr hWnd, int id, uint fsModifiers, uint vk);
    [DllImport("user32.dll")] static extern bool UnregisterHotKey(IntPtr hWnd, int id);

    const uint MOD_ALT = 0x0001, MOD_CONTROL = 0x0002, MOD_SHIFT = 0x0004;
    private readonly IntPtr _hwnd;
    private readonly HwndSource _source;
    private readonly Dictionary<int, Action> _map = new();

    public HotkeyManager(IntPtr hwnd)
    {
        _hwnd = hwnd;
        _source = HwndSource.FromHwnd(hwnd)!;
        _source.AddHook(WndProc);
    }

    public bool Register(int id, uint vk, bool ctrl, bool alt, bool shift, Action action)
    {
        uint mod = 0;
        if (ctrl) mod |= MOD_CONTROL;
        if (alt) mod |= MOD_ALT;
        if (shift) mod |= MOD_SHIFT;
        if (RegisterHotKey(_hwnd, id, mod, vk))
        {
            _map[id] = action;
            return true;
        }
        return false;
    }

    private IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
    {
        const int WM_HOTKEY = 0x0312;
        if (msg == WM_HOTKEY)
        {
            int id = wParam.ToInt32();
            if (_map.TryGetValue(id, out var act)) act();
            handled = true;
        }
        return IntPtr.Zero;
    }

    public void Dispose()
    {
        foreach (var id in _map.Keys.ToList()) UnregisterHotKey(_hwnd, id);
        _source.RemoveHook(WndProc);
    }
}
