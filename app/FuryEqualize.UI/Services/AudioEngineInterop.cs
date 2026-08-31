using System.Runtime.InteropServices;

namespace FuryEqualize.UI.Services;

[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
public struct FuryDeviceInfo
{
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)] public string id;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)] public string name;
    public int channels;
    public int sampleRate;
    public int isDefault;
}

internal static class AudioEngineInterop
{
    private const string DllName = "FuryEqualizeCore.dll";

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_Start(string? deviceId);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_Stop();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_IsRunning();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetPreset(float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetEq(float lowShelfGainDb, float peakGainDb, float highShelfGainDb);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_SetBuffer(int bufferSize);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_GetBuffer();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_SetRenderDevice(string? renderDeviceId);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern int AudioEngine_GetRenderDevice(byte[] outId, int maxLen);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern int AudioEngine_GetDevices([Out] FuryDeviceInfo[] outDevices, int maxCount);

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float AudioEngine_GetInputLevelDb();

    [DllImport(DllName, CallingConvention = CallingConvention.Cdecl)]
    public static extern float AudioEngine_GetGainReductionDb();

    public static bool IsAvailable
    {
        get
        {
            try { _ = AudioEngine_GetBuffer(); return true; }
            catch (DllNotFoundException) { return false; }
            catch (BadImageFormatException) { return false; }
        }
    }
}
